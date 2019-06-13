#define _BSD_SOURCE
#include "orchard-app.h"
#include "orchard-ui.h"

#include "ff.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_l2cap_lld.h"
#include "ble_gattc_lld.h"
#include "ble_gatts_lld.h"
#include "ble_peer.h"
#include "i2s_lld.h"
#include "ides_gfx.h"
#include "crc32.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define OTA_SIZE	BLE_IDES_L2CAP_MTU

typedef struct _OtaHandles {
	FIL		f;
	uint8_t		txbuf[OTA_SIZE];
	uint8_t		retry;
	uint32_t	size;
	uint32_t	last;
	uint32_t	crc;
	uint32_t	send_crc;
	char *		listitems[BLE_PEER_LIST_SIZE + 2];
	ble_gap_addr_t	listaddrs[BLE_PEER_LIST_SIZE + 2];
	OrchardUiContext uiCtx;
	GListener	gl;
} OtaHandles;

static uint32_t
otasend_init (OrchardAppContext *context)
{
	(void)context;

	/*
	 * We don't want any extra stack space allocated for us.
	 * We'll use the heap.
	 */

	return (0);
}

static void
otasend_start (OrchardAppContext *context)
{
	OtaHandles * p;
	ble_peer_entry * peer;
	int i, j;

	gdispClear (Black);

	p = malloc (sizeof (OtaHandles));

	memset (p, 0, sizeof(OtaHandles));
	p->crc = CRC_INIT;

	context->priv = p;

	f_open (&p->f, "0:BADGE.BIN", FA_READ);

	p->listitems[0] = "Choose a peer";
	p->listitems[1] = "Exit";

	blePeerLock ();
	j = 2;
	for (i = 0; i < BLE_PEER_LIST_SIZE; i++) {
		peer = &ble_peer_list[i];
		if (peer->ble_used == 0)
			continue;
		if (peer->ble_isbadge == FALSE)
			continue;
		p->listitems[j] = strdup ((char *)peer->ble_peer_name);
        	p->listaddrs[j].addr_id_peer = TRUE;
        	p->listaddrs[j].addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
		p->listaddrs[j].addr[0] = peer->ble_peer_addr[0];
		p->listaddrs[j].addr[1] = peer->ble_peer_addr[1];
		p->listaddrs[j].addr[2] = peer->ble_peer_addr[2];
		p->listaddrs[j].addr[3] = peer->ble_peer_addr[3];
		p->listaddrs[j].addr[4] = peer->ble_peer_addr[4];
		p->listaddrs[j].addr[5] = peer->ble_peer_addr[5];
		j++;
	}
	blePeerUnlock ();

	p->uiCtx.itemlist = (const char **)p->listitems;
	p->uiCtx.total = j - 1;

	context->instance->ui = getUiByName ("list");
	context->instance->uicontext = &p->uiCtx;
	context->instance->ui->start (context);

	return;
}

static void
otasend_event (OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	OrchardAppRadioEvent *	radio;
	ble_evt_t *		evt;
	UINT			br;
	OtaHandles * 		p;
	char			msg[32];
	ble_gatts_evt_rw_authorize_request_t * rw;
	ble_gatts_evt_write_t * req;
	const OrchardUi * ui;
	OrchardUiContext * uiContext;
	GSourceHandle gs;

	ui = context->instance->ui;
	uiContext = context->instance->uicontext;
	p = context->priv;

	if (event->type == ugfxEvent || event->type == keyEvent) {
		if (ui != NULL)
			ui->event (context, event);
		else {
                        i2sPlay ("sound/click.snd");
                        bleGapDisconnect ();
                        orchardAppExit ();
                        return;
		}
	}

	if (event->type == uiEvent && event->ui.code == uiComplete &&
		event->ui.flags == uiOK && ui != NULL) {

		ui->exit (context);
		context->instance->ui = NULL;

		/* User chose the "EXIT" selection, bail out. */
		if (uiContext->selected == 0) {
			orchardAppExit ();
			return;
		}

		gs = ginputGetMouse (0);
		geventListenerInit (&p->gl);
		geventAttachSource (&p->gl, gs, GLISTEN_MOUSEMETA);
		geventRegisterCallback (&p->gl,
		    orchardAppUgfxCallback, &p->gl);

		putImageFile ("images/fwupdate.rgb", 0, 0);

		/*
		 * Initiate GAP connection to peer.
		 */

		bleGapConnect (&p->listaddrs[uiContext->selected + 1]);
	}

        if (event->type == radioEvent) {
                radio = (OrchardAppRadioEvent *)&event->radio;
                evt = &radio->evt;

		switch (radio->type) {
			/*
			 * GAP connection achieved. Now issue a GATTC write to
			 * the OTA GATTS characteristic to indicate an offer of
			 * a firmware update.
			 */
			case connectEvent:
				screen_alert_draw (FALSE, "Handshaking...");
				msg[0] = BLE_IDES_OTAUPDATE_OFFER;
				bleGattcWrite (ot_handle.value_handle,
				    (uint8_t *)msg, 1, FALSE);
				break;

			/*
			 * GATTS write event - this should be a response from
			 * the peer either accepting or declining the uptate.
			 */
			case gattsReadWriteAuthEvent:
				rw =
				 &evt->evt.gatts_evt.params.authorize_request;
				req = &rw->request.write;
	 			if (rw->request.write.handle ==
				    ot_handle.value_handle &&
				    req->data[0] == BLE_IDES_OTAUPDATE_ACCEPT){
					screen_alert_draw (FALSE,
					    "Offer accepted...");
					/* Offer accepted - create L2CAP link*/
					bleL2CapConnect (BLE_IDES_OTA_PSM);
				} else {
					screen_alert_draw (FALSE,
					    "Offer declined");
					chThdSleepMilliseconds (2000);
					orchardAppExit ();
				}
				break;

			/* L2CAP connected -- start sending the file */
			case l2capConnectEvent:
				screen_alert_draw (FALSE, "Sending...");
				/* FALLTHROUGH */
			/* L2CAP TX successful -- send the next chunk */
			case l2capTxEvent:

				if (p->retry == 0) {
					br = 0;
					f_read (&p->f, p->txbuf, OTA_SIZE,&br);

					/*
					 * The only time the number of bytes
					 * read will be less than OTA_SIZE
					 * will be when we read the last few
					 * bytes left in the file. The other
					 * side expects that a transmission of
					 * 4 bytes will always contain the
				 	 * checksum. If there happens to be
					 * exactly 4 bytes left in the file
					 * after the last OTA_SIZE chunk, then
					 * the other side will mistake it for
					 * the checksum. To avoid this, if
					 * we have exactly 4 bytes left,
					 * lie and claim it's 5 just so the
					 * other side doesn't get confused.
					 */

					if (br == 4)
						br++;
					if (br == 0 && p->send_crc == 0) {
						memcpy (p->txbuf, &p->crc, 4);
						br = 4;
						p->send_crc = 1;
					} else {
						p->crc = crc32_le (p->txbuf,
						    br, p->crc);
					}
					p->last = br;
					p->size += br;
				} else
					br = p->last;

				if (br) {
					/* Send data chunk */
					if (bleL2CapSend (p->txbuf, br) ==
					    NRF_SUCCESS) {
						p->retry = 0;
						sprintf (msg, "Sent %ld bytes",
						    p->size);
						screen_alert_draw (FALSE, msg);
					} else
						p->retry = 1;
				} else {
					if (radio->type == l2capTxEvent) {
						screen_alert_draw (FALSE,
						    "Done!");
						orchardAppExit ();
					}
				}
				break;

			/* For any of these events, bail out. */
			case l2capConnectRefusedEvent:
			case l2capDisconnectEvent:
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
otasend_exit (OrchardAppContext *context)
{
	OtaHandles *		p;
	int 			i;
	const OrchardUi * ui;

	ui = context->instance->ui;

	if (ui != NULL) {
		ui->exit (context);
		context->instance->ui = NULL;
	}

	bleGapDisconnect ();

	p = context->priv;

	f_close (&p->f);

	geventRegisterCallback (&p->gl, NULL, NULL);
	geventDetachSource (&p->gl, NULL);

	for (i = 0; i < BLE_PEER_LIST_SIZE; i++) {
		if (p->listitems[i + 2] != NULL)
			free (p->listitems[i + 2]);
	}

	free (p);

	return;
}

orchard_app("OTA Send", "icons/wheel.gif",
    0, otasend_init, otasend_start, otasend_event, otasend_exit, 1);
