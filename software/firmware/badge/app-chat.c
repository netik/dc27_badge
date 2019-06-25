/*-
 * Copyright (c) 2016-2017
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

#include <stdio.h>
#include <stdlib.h>

#include "orchard-app.h"
#include "orchard-ui.h"
#include "fontlist.h"
#include "ides_gfx.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_l2cap_lld.h"
#include "ble_gattc_lld.h"
#include "ble_gatts_lld.h"
#include "ble_peer.h"

#include "nrf52i2s_lld.h"

#include "badge.h"

#include <stdlib.h>
#include <string.h>

#define MAX_PEERS	50
#define PEER_ADDR_LEN	12 + 5
#define MAX_PEERMEM	(PEER_ADDR_LEN + BLE_GAP_ADV_SET_DATA_SIZE_EXTENDED_MAX_SUPPORTED + 1)

typedef struct _ChatHandles {
	char *			peernames[MAX_PEERS + 2];
	ble_gap_addr_t		peeraddrs[MAX_PEERS + 2];
	OrchardUiContext	uiCtx;
	char			txbuf[MAX_PEERMEM + BLE_IDES_L2CAP_MTU + 3];
	char			rxbuf[BLE_IDES_L2CAP_MTU];
	uint8_t 		peers;
	int			peer;
	ble_gap_addr_t		netid;
	uint16_t		cid;
	uint8_t			uuid[16];
	uint16_t		len;
} ChatHandles;

extern orchard_app_instance instance;

static int
insert_peer (OrchardAppContext * context, ble_evt_t * evt,
    OrchardAppRadioEvent * radio)
{
	ChatHandles *	p;
	uint8_t	*	name;
	uint8_t 	len;
	ble_gap_addr_t *	addr;
	int i;

	p = context->priv;

	/* If the peer list is full, return */

	if (p->peers == (MAX_PEERS + 2))
		return (-1);

	name = radio->pkt;
        len = radio->pktlen;
	addr = &evt->evt.gap_evt.params.adv_report.peer_addr;

	if (bleGapAdvBlockFind (&name, &len,
	    BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME) != NRF_SUCCESS) {
		name = (uint8_t *)"";
		len = 0;
	} else
		name[len] = 0x00;

	for (i = 2; i < p->peers; i++) {
		if (memcmp (&p->peeraddrs[i], addr,
		    sizeof (ble_gap_addr_t)) == 0) {
			/* Already an entry for this badge, just return. */
			return (-1);
		}
	}

	/* No match, add a new entry */

	memcpy (&p->peeraddrs[p->peers], addr, sizeof (ble_gap_addr_t));

	p->peernames[p->peers] = malloc (MAX_PEERMEM);

	if (len)
		snprintf (p->peernames[p->peers], MAX_PEERMEM, "%s", name);
	else
		snprintf (p->peernames[p->peers], MAX_PEERMEM,
		    "%02x:%02x:%02x:%02x:%02x:%02x",
		    addr->addr[5], addr->addr[4], addr->addr[3],
		    addr->addr[2], addr->addr[1], addr->addr[0]);

	p->peers++;

	return (0);
}

static void
chat_session_start (OrchardAppContext * context)
{
	ChatHandles * p;
	const OrchardUi * ui;

	p = context->priv;

	p->peernames[0] = "Type @ or press button to exit";
	memset (p->txbuf, 0, sizeof(p->txbuf));
	p->peernames[1] = p->txbuf;

	/* Terminate the list UI. */

	ui = context->instance->ui;
	if (ui != NULL)
		ui->exit (context);

	/* Load the keyboard UI. */

	p->uiCtx.itemlist = (const char **)p->peernames;
	p->uiCtx.total = BLE_IDES_L2CAP_MTU - 1;
	context->instance->ui = getUiByName ("keyboard");
	context->instance->uicontext = &p->uiCtx;
       	context->instance->ui->start (context);
	return;
}

static uint32_t
chat_init (OrchardAppContext *context)
{
	(void)context;
	return (0);
}

static void
chat_start (OrchardAppContext *context)
{
	(void)context;

	return;
}

static void
chat_event (OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	ble_gatts_evt_rw_authorize_request_t * rw;
	ble_gatts_evt_write_t * req;
	OrchardUiContext * uiContext;
	OrchardAppRadioEvent * radio;
	ble_evt_t * evt;
	const OrchardUi * ui;
	ChatHandles * p;
	OrchardAppEvent * e;
	ble_peer_entry * peer;
	int i;

	/*
	 * We're cheating a bit here by ignoring the 'const'
	 * qualifier for the event handle, but re-using the
	 * current event saves us from having to consume extra
	 * stack space for the UI event.
	 */

	e = (OrchardAppEvent *)event;

	/*
	 * In order to avoid any chance of a race condition between
	 * the start end exit routines and the event notifications,
	 * we have to handle context creation and destruction here
	 * in the event handler. This forces the entire app to be
	 * serialized through the event handler thread.
	 */

	if (event->type == appEvent) {
		if (event->app.event == appStart) {
#ifdef notdef
			if (airplane_mode_check() == true)
				return;
#endif
			p = malloc (sizeof(ChatHandles));
			memset (p, 0, sizeof(ChatHandles));
			p->cid = BLE_L2CAP_CID_INVALID;
			p->peers = 2;

			context->priv = p;

			if (ble_gap_role == BLE_GAP_ROLE_PERIPH) {
				p->peer = 2;
				p->peers++;
				p->peernames[p->peer] = malloc (MAX_PEERMEM);
        			peer = blePeerFind (ble_peer_addr.addr);

				if (peer == NULL)
					snprintf (p->peernames[p->peer],
					    MAX_PEERMEM,
					    "%X:%X:%X:%X:%X:%X",
					    ble_peer_addr.addr[5],
					    ble_peer_addr.addr[4],
					    ble_peer_addr.addr[3],
					    ble_peer_addr.addr[2],
					    ble_peer_addr.addr[1],
					    ble_peer_addr.addr[0]);
				else
					strcpy (p->peernames[p->peer],
					    (char *)peer->ble_peer_name);

				chat_session_start (context);
				return;
			}

			p->peernames[0] = "Scanning for users...";
			p->peernames[1] = "Exit";

			p->uiCtx.itemlist = (const char **)p->peernames;
			p->uiCtx.total = 1;

			context->instance->ui = getUiByName ("list");
			context->instance->uicontext = &p->uiCtx;
       			context->instance->ui->start (context);
		}

		if (event->app.event == appTerminate) {
			ui = context->instance->ui;
			if (ui != NULL)
				ui->exit (context);
			p = context->priv;
			for (i = 2; i < p->peers; i++) {
				if (p->peernames[i] != NULL)
					free (p->peernames[i]);
			}
			if (p->cid != BLE_L2CAP_CID_INVALID)
				bleL2CapDisconnect (p->cid);
			bleGapDisconnect ();
			free (p);
			context->priv = NULL;
		}
		return;
	}

	p = context->priv;

	/* Shouldn't happen, but check anyway. */

	if (p == NULL)
		return;

	ui = context->instance->ui;
	uiContext = context->instance->uicontext;

 	if (event->type == keyEvent && event->key.flags == keyPress) {
		if (ui == getUiByName ("keyboard")) {
			orchardAppExit ();
			return;
		}
	}

	if (ui != NULL &&
	    (event->type == ugfxEvent || event->type == keyEvent))
		ui->event (context, event);

	if (event->type == radioEvent) {
		radio = (OrchardAppRadioEvent *)&event->radio;
		evt = &radio->evt;

		/* A chat message arrived, update the text display. */

		if (radio->type == l2capRxEvent) {
			snprintf (p->rxbuf, sizeof(p->rxbuf), "<%s> %s",
			    p->peernames[p->peer], radio->pkt);
			p->peernames[0] = p->rxbuf;
			/* Tell the keyboard UI to redraw */
			e->type = uiEvent;
			ui->event (context, e);
			return;
		}

		/* A new advertisement has arrived, update the user list */

		if (radio->type == advAndScanEvent) {
			if (ui == getUiByName ("list") &&
			    insert_peer (context, evt, radio) != -1) {
				uiContext->selected = p->peers - 1;
				e->type = uiEvent;
				e->ui.flags = uiOK;
				ui->event (context, e);
			}
			return;
		}

		/*
		 * Connection was successful. There are two cases where we
		 * can get a GAP connect event:
		 *
		 * 1) we're trying to connect to a peer
	 	 * 2) we're the peer and we accept a connection
		 *
		 * In the first case, our role is "central", and now
		 * we have to check to see if the peer supports badge
		 * services by checking for our UUID (since we can't talk
		 * to anything that's not a badge).
		 */

		if (radio->type == connectEvent) {
			screen_alert_draw (FALSE, "Connected to peer...");
			if (ble_gap_role == BLE_GAP_ROLE_CENTRAL) {
				p->len = sizeof(p->uuid);
				bleGattcRead (ble_gatts_ides_handle,
				     p->uuid, &p->len, FALSE);
			}
			return;
		}

		/*
		 * GATTC read was successful. Check the UUID and make sure
		 * it matches what we expect, otherwise we're not talking
		 * a badge. If we are talking to a badge, then write to the
		 * peer's chat request service to notify it of our desire
		 * to engage them in spirited conversation.
		 */

		if (radio->type == gattcCharReadEvent) {
			if (p->len != 16 ||
			    memcmp (p->uuid, ble_ides_base_uuid.uuid128, 16)) {
				screen_alert_draw (FALSE,
				    "Peer is not a badge!");
				chThdSleepMilliseconds (1000);
				ui = context->instance->ui;
				if (ui != NULL)
					ui->exit (context);
				orchardAppExit ();
			}

			p->uuid[0] = BLE_IDES_CHATREQ_REQUEST;
			bleGattcWrite (ch_handle.value_handle,
			    p->uuid, 1, FALSE);

			if (ble_gap_role == BLE_GAP_ROLE_CENTRAL) {
				if (bleL2CapConnect (BLE_IDES_CHAT_PSM) !=
				    NRF_SUCCESS) {
					screen_alert_draw (FALSE,
					    "L2CAP Connection refused!");
					chThdSleepMilliseconds (1000);
					ui = context->instance->ui;
					if (ui != NULL)
						ui->exit (context);
					orchardAppExit ();
				}
				screen_alert_draw (FALSE,
				    "L2CAP Connecting...");
			} else
				screen_alert_draw (FALSE, "Wrong role?");
		
			return;
		}

		if (radio->type == l2capConnectEvent) {
			screen_alert_draw (FALSE, "Channel open!");
			chThdSleepMilliseconds (1000);
			p->cid = evt->evt.l2cap_evt.local_cid;
			chat_session_start (context);
			return;
		}

		if (radio->type == gattsReadWriteAuthEvent) {
			rw = &evt->evt.gatts_evt.params.authorize_request;
			if (rw->type == BLE_GATTS_AUTHORIZE_TYPE_WRITE) {
				req = &rw->request.write;
				if (req->data[0] == BLE_IDES_CHATREQ_DECLINE) {
					screen_alert_draw (TRUE,
					    "Chat request declined!");
					i2sPlay ("sound/wilhelm.snd");
					chThdSleepMilliseconds (1000);
				} else
					return;
			}
			return;
		}

		if (radio->type == l2capTxEvent ||
		    radio->type == l2capTxDoneEvent ||
		    radio->type == l2capDisconnectEvent ||
		    radio->type == gattcCharWriteEvent ||
		    radio->type == advertisementEvent ||
		    radio->type == scanResponseEvent)
			return;

		if (radio->type == connectTimeoutEvent) {
			screen_alert_draw (FALSE, "Connection timed out!");
		}

		if (radio->type == l2capConnectRefusedEvent) {
			screen_alert_draw (FALSE, "Connection refused!");
		}

		if (radio->type == disconnectEvent) {
			screen_alert_draw (TRUE, "Connection closed!");
		}

		chThdSleepMilliseconds (1000);
		orchardAppExit ();
		return;
	}

	if (event->type == uiEvent &&
	    event->ui.code == uiComplete &&
	    event->ui.flags == uiOK) {
		/*
		 * If this is the list ui exiting, it means we chose a
		 * user to chat with. Now switch to the keyboard ui.
		 */
		if (ui == getUiByName ("list")) {
			/* User chose the "EXIT" selection, bail out. */

			if (uiContext->selected == 0) {
				orchardAppExit ();
				return;
			}

			ui->exit (context);
			context->instance->ui = NULL;
			context->instance->uicontext = NULL;

			p->peer = uiContext->selected + 1;
			memcpy (&p->netid, &p->peeraddrs[p->peer],
			    sizeof(p->netid));

			/*
			 * Now that we chose a peer, release any memory
			 * for the other peers.
			 */

			for (i = 2; i < p->peers; i++) {
				if (i == p->peer)
					continue;
				if (p->peernames[i] != NULL) {
					free (p->peernames[i]);
					p->peernames[i] = NULL;
				}
			}

			/* Connect to the user */

			if (bleGapConnect (&p->netid) != NRF_SUCCESS)
				orchardAppExit ();

			screen_alert_draw (FALSE, "Connecting...");
		} else {
			/* 0xFF means exit chat */
			if (uiContext->total == 0xFF) {
				orchardAppExit ();
			} else {
				p->txbuf[uiContext->selected] = 0x0;
				if (bleL2CapSend ((uint8_t *)p->txbuf,
				    strlen (p->txbuf) + 1) != NRF_SUCCESS) {
					orchardAppExit ();
					return;
				}
				memset (p->txbuf, 0, sizeof(p->txbuf));
				p->uiCtx.total = BLE_IDES_L2CAP_MTU - 1;
				/* Tell the keyboard UI to redraw */
				e->type = uiEvent;
				e->ui.flags = uiCancel;
				ui->event (context, e);
			}
		}
	}

	return;
}

static void chat_exit (OrchardAppContext *context)
{
	(void)context;
	return;
}

orchard_app("Radio Chat", "icons/lghthous.rgb", 0,
	chat_init, chat_start, chat_event, chat_exit, 9999);
