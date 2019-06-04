#include "orchard-app.h"
#include "orchard-ui.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_gattc_lld.h"
#include "ble_gatts_lld.h"
#include "i2s_lld.h"
#include "ides_gfx.h"
#include "ff.h"

#include <stdio.h>
#include <stdint.h>

typedef struct _OtaHandles {
	FIL			f;
	uint32_t		total;
	int			status;
	GListener		gl;
} OtaHandles;
	
static uint32_t
ota_init (OrchardAppContext *context)
{
	(void)context;

	/*
	 * We don't want any extra stack space allocated for us.
	 * We'll use the heap.
	 */

	return (0);
}

static void
ota_start (OrchardAppContext *context)
{
	OtaHandles * p;
	GSourceHandle gs;

	gdispClear (Black);

	p = malloc (sizeof (OtaHandles));
	context->priv = p;
	p->total = 0;
	p->status = 0;

	gs = ginputGetMouse (0);
	geventListenerInit (&p->gl);
	geventAttachSource (&p->gl, gs, GLISTEN_MOUSEMETA);
	geventRegisterCallback (&p->gl, orchardAppUgfxCallback, &p->gl);

	if (f_open (&p->f, "0:BADGE.BIN", FA_WRITE) != FR_OK) {
		screen_alert_draw (FALSE, "Opening firmware file failed!");
		chThdSleepMilliseconds (2000);
		orchardAppExit ();
	} else {
		screen_alert_draw (FALSE, "Starting...");
	}

	return;
}

static void
ota_event (OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	OrchardAppRadioEvent *	radio;
	char			buf[32];
	uint8_t			reply;
	UINT			br;
	OtaHandles *		p;
	GEventMouse *		me;

	p = context->priv;

	/* Send accept response to OTA sender */
	if (event->type == appEvent && event->app.event == appStart) {
		reply = BLE_IDES_OTAUPDATE_ACCEPT;
		bleGattcWrite (ot_handle.value_handle, &reply, 1, FALSE);
		screen_alert_draw (FALSE, "Handshaking...");
	}

	if (event->type == keyEvent) {
		p->status = -1;
		bleGapDisconnect ();
		i2sPlay ("sound/click.snd");
		orchardAppExit ();
		return;
	}

	if (event->type == ugfxEvent) {
		me = (GEventMouse *)event->ugfx.pEvent;
		if (me->buttons & GMETA_MOUSE_DOWN) {
			i2sPlay ("sound/click.snd");
			p->status = -1;
			bleGapDisconnect ();
			orchardAppExit ();
			return;
		}
	}

	if (event->type == radioEvent) {
		radio = (OrchardAppRadioEvent *)&event->radio;

		switch (radio->type) {
			/* L2CAP link established */
			case l2capConnectEvent:
				screen_alert_draw (FALSE, "Receiving...");
				break;
			/* Data received, write it to the file */
			case l2capRxEvent:
				p->total += radio->pktlen;
				sprintf (buf, "Received %ld bytes", p->total);
				screen_alert_draw (FALSE, buf);
				if (f_write (&p->f, radio->pkt,
				    radio->pktlen, &br) != FR_OK) {
					screen_alert_draw (FALSE,
					    "Writing failed!");
					bleGapDisconnect ();
					p->status = -1;
					orchardAppExit ();
				}
				break;
			/* L2CAP link closed -- transfer complete */
			case l2capDisconnectEvent:
				sprintf (buf, "Received %ld bytes", p->total);
				screen_alert_draw (FALSE, buf);
				chThdSleepMilliseconds (3000);
				break;
			/* For any of these events, bail out. */
			case l2capConnectRefusedEvent:
			case disconnectEvent:
			case connectTimeoutEvent:
				orchardAppExit ();
				break;
			default:
				break;
		}
	}

	return;
}

static void
ota_exit (OrchardAppContext *context)
{
	OtaHandles * p;
	int status;

	p = context->priv;
	status = p->status;
	f_close (&p->f);
	geventRegisterCallback (&p->gl, NULL, NULL);
	geventDetachSource (&p->gl, NULL);

	free (p);

	/* Launch the firmware updater */

	if (status == 0)
		orchardAppRun (orchardAppByName ("Update FW"));

	return;
}

orchard_app("OTA Recv", "icons/wheel.rgb", APP_FLAG_HIDDEN,
    ota_init, ota_start, ota_event, ota_exit, 1);
