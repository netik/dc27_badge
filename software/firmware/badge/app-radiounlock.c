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

#define _BSD_SOURCE
#include "orchard-app.h"
#include "orchard-ui.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_l2cap_lld.h"
#include "ble_gattc_lld.h"
#include "ble_gatts_lld.h"
#include "ble_peer.h"
#include "i2s_lld.h"
#include "ides_gfx.h"
#include "unlocks.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

extern char * unlock_desc[];

#define UL_STATE_PW_SENT	1
#define UL_STATE_UL_SENT	2
#define UL_STATE_UL_READ	3

typedef struct _UlHandles {
	uint32_t	unlocks;
	int		unlock_idx;
	uint16_t	len;
	uint32_t	ul_state;
	char *		listitems[BLE_PEER_LIST_SIZE + 2];
	ble_gap_addr_t	listaddrs[BLE_PEER_LIST_SIZE + 2];
	OrchardUiContext uiCtx;
	GListener	gl;
} UlHandles;

static uint32_t
radiounlock_init (OrchardAppContext *context)
{
	(void)context;

	/*
	 * We don't want any extra stack space allocated for us.
	 * We'll use the heap.
	 */

	return (0);
}

static void
radiounlock_start (OrchardAppContext *context)
{
	UlHandles * p;
	ble_peer_entry * peer;
	int i, j;

	gdispClear (Black);

	p = malloc (sizeof (UlHandles));

	memset (p, 0, sizeof(UlHandles));

	context->priv = p;

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
radiounlock_event (OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	OrchardAppRadioEvent *	radio;
	UlHandles * 		p;
	const OrchardUi * ui;
	OrchardUiContext * uiContext;
	GSourceHandle gs;

	ui = context->instance->ui;
	uiContext = context->instance->uicontext;
	p = context->priv;

	if ((event->type == ugfxEvent || event->type == keyEvent) &&
	    ui != NULL) {
		ui->event (context, event);
		return;
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

		putImageFile ("images/runlock.rgb", 0, 0);

		/*
		 * Initiate GAP connection to peer.
		 */

		bleGapConnect (&p->listaddrs[uiContext->selected + 1]);
	}

	if (event->type == keyEvent && event->key.flags == keyPress) {
		i2sPlay ("sound/click.snd");

		if (event->key.code == keyAUp ||
		    event->key.code == keyBUp) {
			/*
			 * Note: We allow one index higher than MAX_ULCODES
			 * so that we can propagate the black badge
			 * attribute.
			 */
			if (p->unlock_idx == MAX_ULCODES)
				return;
			else
				p->unlock_idx++;
			screen_alert_draw (FALSE, unlock_desc[p->unlock_idx]);
		}

		if (event->key.code == keyADown ||
		    event->key.code == keyBDown) {
			if (p->unlock_idx == 0)
				return;
			else
				p->unlock_idx--;
			screen_alert_draw (FALSE, unlock_desc[p->unlock_idx]);
		}

		if (event->key.code == keyASelect ||
		    event->key.code == keyBSelect) {
			p->unlocks |= __builtin_bswap32 (1 << p->unlock_idx);
			bleGattcWrite (ul_handle.value_handle,
			    (uint8_t *)&p->unlocks, 4, FALSE);
			screen_alert_draw (FALSE, "Sending unlock...");
			p->ul_state = UL_STATE_UL_SENT;
		}
	}

        if (event->type == radioEvent) {
                radio = (OrchardAppRadioEvent *)&event->radio;

		switch (radio->type) {

			/*
			 * GAP connection achieved. Now issue a GATTC write to
			 * the send the password to unlock remote updates.
			 */

			case connectEvent:

				screen_alert_draw (FALSE,
				    "Sending password...");
				chThdSleepMilliseconds (100);
				bleGattcWrite (pw_handle.value_handle,
				    (uint8_t *)BLE_IDES_PASSWORD,
				    strlen (BLE_IDES_PASSWORD), FALSE);
				p->ul_state = UL_STATE_PW_SENT;
				break;

			case gattcCharWriteEvent:

				if (p->ul_state == UL_STATE_PW_SENT) {
					screen_alert_draw (FALSE,
					    "Password sent!");
					p->ul_state = UL_STATE_UL_READ;
					p->len = 4;
					bleGattcRead (ul_handle.value_handle,
			    		    (uint8_t *)&p->unlocks,
					    &p->len, FALSE);
					screen_alert_draw (FALSE,
					    "Fetching unlocks...");
				}

				if (p->ul_state == UL_STATE_UL_READ) {
					screen_alert_draw (FALSE,
					    "Unlocks received!");
					p->ul_state = 0;
					p->unlocks =
					    __builtin_bswap32 (p->unlocks);
					screen_alert_draw (FALSE,
					    unlock_desc[p->unlock_idx]);
				}

				if (p->ul_state == UL_STATE_UL_SENT) {
					screen_alert_draw (FALSE,
					    "Unlock sent!");
					chThdSleepMilliseconds (2000);
					orchardAppExit ();
				}
				break;

			/* For any of these events, bail out. */

			case gattsReadWriteAuthEvent:
			case l2capConnectEvent:
			case l2capTxEvent:
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
radiounlock_exit (OrchardAppContext *context)
{
	UlHandles *		p;
	int 			i;
	const OrchardUi * ui;

	ui = context->instance->ui;

	if (ui != NULL) {
		ui->exit (context);
		context->instance->ui = NULL;
	}

	bleGapDisconnect ();

	p = context->priv;

	geventRegisterCallback (&p->gl, NULL, NULL);
	geventDetachSource (&p->gl, NULL);

	for (i = 0; i < BLE_PEER_LIST_SIZE; i++) {
		if (p->listitems[i + 2] != NULL)
			free (p->listitems[i + 2]);
	}

	free (p);

	return;
}

orchard_app("Radio Unlock", "icons/wheel.rgb", APP_FLAG_BLACKBADGE,
    radiounlock_init, radiounlock_start, radiounlock_event,
    radiounlock_exit, 2);
