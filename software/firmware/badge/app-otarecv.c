/*-
 * Copyright (c) 2019
 *      Bill Paul <wpaul@windriver.com>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "orchard-app.h"
#include "orchard-ui.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_gattc_lld.h"
#include "ble_gatts_lld.h"
#include "i2s_lld.h"
#include "ides_gfx.h"
#include "ff.h"
#include "crc32.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct _OtaHandles {
	FIL			f;
	uint32_t		total;
	uint32_t		crc;
	uint32_t		sent_crc;
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

	putImageFile ("images/fwupdate.rgb", 0, 0);

	p = malloc (sizeof (OtaHandles));
	context->priv = p;
	p->total = 0;
	p->status = 0;
	p->crc = CRC_INIT;

	gs = ginputGetMouse (0);
	geventListenerInit (&p->gl);
	geventAttachSource (&p->gl, gs, GLISTEN_MOUSEMETA);
	geventRegisterCallback (&p->gl, orchardAppUgfxCallback, &p->gl);

	f_unlink ("OTA.BIN");
	if (f_open (&p->f, "OTA.BIN", FA_WRITE|FA_CREATE_NEW) != FR_OK) {
		screen_alert_draw (FALSE, "Opening firmware file failed!");
		chThdSleepMilliseconds (2000);
		p->status = -1;
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
		i2sPlay ("sound/click.snd");
		p->status = -1;
		orchardAppExit ();
		return;
	}

	if (event->type == ugfxEvent) {
		me = (GEventMouse *)event->ugfx.pEvent;
		if (me->buttons & GMETA_MOUSE_DOWN) {
			i2sPlay ("sound/click.snd");
			p->status = -1;
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
				/*
				 * If we receive exactly 4 bytes, it's the
				 * data checksum, and we don't need to write
				 * it to the file.
				 */
				if (radio->pktlen == 4)
					memcpy (&p->sent_crc, radio->pkt, 4);
				else {
					p->crc = crc32_le (radio->pkt,
					    radio->pktlen, p->crc);
					if (f_write (&p->f, radio->pkt,
					    radio->pktlen, &br) != FR_OK) {
						screen_alert_draw (FALSE,
						    "Writing failed!");
						p->status = -1;
						orchardAppExit ();
					}
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
	uint32_t crc, sent_crc;

	p = context->priv;
	status = p->status;
	crc = p->crc;
	sent_crc = p->sent_crc;
	f_close (&p->f);
	geventRegisterCallback (&p->gl, NULL, NULL);
	geventDetachSource (&p->gl, NULL);

	free (p);

	/* Launch the firmware updater */

	if (status == 0) {
		if (crc != sent_crc) {
			screen_alert_draw (FALSE,
			    "Bad checksum - aborting");
			f_unlink ("OTA.BIN");
			chThdSleepMilliseconds (2000);
		} else {
			f_unlink ("BADGE.BIN");
			f_rename ("OTA.BIN", "BADGE.BIN");
			orchardAppRun (orchardAppByName ("Update FW"));
		}
	} else
		bleGapDisconnect ();

	return;
}

orchard_app("OTA Recv", NULL, APP_FLAG_HIDDEN,
    ota_init, ota_start, ota_event, ota_exit, 1);
