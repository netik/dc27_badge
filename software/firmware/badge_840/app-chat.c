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

#include "orchard-app.h"
#include "orchard-ui.h"
#include "fontlist.h"
#include "ides_gfx.h"
#include "chprintf.h"

#include "ble_gap_lld.h"

#include "badge.h"

#include <stdlib.h>
#include <string.h>

#define MAX_PEERS	50
#define PEER_ADDR_LEN	12 + 5
#define MAX_PEERMEM	(PEER_ADDR_LEN + BLE_GAP_ADV_SET_DATA_SIZE_EXTENDED_MAX_SUPPORTED + 1)

typedef struct _ChatHandles {
	char *			listitems[MAX_PEERS + 2];
	OrchardUiContext	uiCtx;
	char			txbuf[MAX_PEERMEM + BLE_IDES_L2CAP_MTU + 3];
	char			rxbuf[BLE_IDES_L2CAP_MTU];
	uint8_t 		peers;
	int			peer;
	ble_gap_addr_t		netid;
	uint16_t		cid;
} ChatHandles;

extern orchard_app_instance instance;

static int
insert_peer (OrchardAppContext * context, ble_evt_t * evt,
    OrchardAppRadioEvent * radio)
{
	ChatHandles *	p;
	int		i;
	char 		peerid[PEER_ADDR_LEN + 1];
	uint8_t	*	name;
	uint8_t 	len;
	ble_gap_addr_t *	addr;

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

	chsnprintf (peerid, sizeof(peerid), "%02x:%02x:%02x:%02x:%02x:%02x",
	    addr->addr[5], addr->addr[4], addr->addr[3],
	    addr->addr[2], addr->addr[1], addr->addr[0]);

	for (i = 2; i < p->peers; i++) {
		/* Already an entry for this badge, just return. */
		if (strncmp (p->listitems[i], peerid, PEER_ADDR_LEN) == 0)
			return (-1);
	}

	/* No match, add a new entry */

	p->listitems[p->peers] = chHeapAlloc (NULL, MAX_PEERMEM);
	chsnprintf (p->listitems[p->peers], MAX_PEERMEM, "%s:%s",
	    peerid, name);

	p->peers++;

	return (0);
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
	OrchardUiContext * uiContext;
	OrchardAppRadioEvent * radio;
	ble_evt_t * evt;
	const OrchardUi * ui;
	ChatHandles * p;
	OrchardAppEvent * e;
	unsigned int addr[6];
	char * c;
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
			p = chHeapAlloc(NULL, sizeof(ChatHandles));
			memset (p, 0, sizeof(ChatHandles));
			p->peers = 2;
#ifdef notdef
			if (airplane_mode_check() == true)
				return;
#endif
			p->listitems[0] = "Scanning for users...";
			p->listitems[1] = "Exit";

			p->uiCtx.itemlist = (const char **)p->listitems;
			p->uiCtx.total = 1;

			context->instance->ui = getUiByName ("list");
			context->instance->uicontext = &p->uiCtx;
       			context->instance->ui->start (context);

			p->cid = BLE_L2CAP_CID_INVALID;

			context->priv = p;
		}
		if (event->app.event == appTerminate) {
			ui = context->instance->ui;
			if (ui != NULL)
				ui->exit (context);
			p = context->priv;
			for (i = 2; i < p->peers; i++) {
				if (p->listitems[i] != NULL)
					chHeapFree (p->listitems[i]);
			}
			if (p->cid != BLE_L2CAP_CID_INVALID)
				bleL2CapDisconnect (p->cid);
			bleGapDisconnect ();
			chHeapFree (p);
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

	if (event->type == ugfxEvent || event->type == keyEvent)
		ui->event (context, event);

	if (event->type == radioEvent) {
		radio = (OrchardAppRadioEvent *)&event->radio;
		evt = &radio->evt;

		/* A chat message arrived, update the text display. */

		if (radio->type == l2capRxEvent) {
			chsnprintf (p->rxbuf, sizeof(p->rxbuf), "<%s> %s",
			    p->listitems[p->peer], radio->pkt);
			p->listitems[0] = p->rxbuf;
			/* Tell the keyboard UI to redraw */
			e->type = uiEvent;
			ui->event (context, e);
			return;
		}

		/* A new advertisement has arrived, update the user list */

		if (radio->type == advertisementEvent) {
			if (ui == getUiByName ("list") &&
			    insert_peer (context, evt, radio) != -1) {
				uiContext->selected = p->peers - 1;
				e->type = uiEvent;
				e->ui.flags = uiOK;
				ui->event (context, e);
			}
			return;
		}

		if (radio->type == connectEvent) {
			screen_alert_draw (FALSE, "Connected to peer...");

			/*
			 * There are two cases where we can get a GAP
			 * connect event:
			 * 1) we're trying to connect to a peer
		 	 * 2) we're the peer and we accept a connection
			 * In the first case, our role is "central"
			 * and in the second our role is "peripheral."
			 * Once the GAP connection is made, only one
			 * side needs to establish an L2CAP connection,
			 * so we only do that if we're the central.
			 */

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
			}
			return;
		}

		if (radio->type == l2capConnectEvent) {
			screen_alert_draw (FALSE, "Channel open!");
			chThdSleepMilliseconds (1000);
			p->cid = evt->evt.l2cap_evt.local_cid;
			p->listitems[0] = "Type @ or press button to exit";
			memset (p->txbuf, 0, sizeof(p->txbuf));
			p->listitems[1] = p->txbuf;

			/* Terminate the list UI. */

			ui = context->instance->ui;
			if (ui != NULL)
				ui->exit (context);

			/* Load the keyboard UI. */

			p->uiCtx.itemlist = (const char **)p->listitems;
			p->uiCtx.total = BLE_IDES_L2CAP_MTU - 1;
			context->instance->ui = getUiByName ("keyboard");
			context->instance->uicontext = &p->uiCtx;
       			context->instance->ui->start (context);
			return;
		}

		if (radio->type == l2capTxEvent ||
		    radio->type == l2capTxDoneEvent)
			return;

		if (radio->type == connectTimeoutEvent) {
			screen_alert_draw (FALSE, "Connection timed out!");
		}

		if (radio->type == l2capConnectRefusedEvent) {
			screen_alert_draw (FALSE, "Connection refused!");
		}

		if (radio->type == disconnectEvent ||
		    radio->type == l2capDisconnectEvent) {
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

			/*
			 * This is a little messy. Now that we've chosen
			 * a user to talk to, we need their netid to use
			 * as the destination address for the radio packet
			 * header. We don't want to waste the RAM to save
			 * a netid in addition to the name string, so instead
			 * we recover the netid of the chosen user from
			 * the name string and cache it.
			 */

			p->peer = uiContext->selected + 1;
			c = &p->listitems[p->peer][PEER_ADDR_LEN];
			*c = '\0';
			sscanf (p->listitems[p->peer],
			    "%02x:%02x:%02x:%02x:%02x:%02x",
			    &addr[5], &addr[4], &addr[3],
			    &addr[2], &addr[1], &addr[0]);
			*c = ':';
			p->netid.addr_id_peer = TRUE;
			p->netid.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
			p->netid.addr[0] = addr[0];
			p->netid.addr[1] = addr[1];
			p->netid.addr[2] = addr[2];
			p->netid.addr[3] = addr[3];
			p->netid.addr[4] = addr[4];
			p->netid.addr[5] = addr[5];

			/*
			 * Now that we chose a peer, release any memory
			 * for the other peers.
			 */

			for (i = 2; i < p->peers; i++) {
				if (i == p->peer)
					continue;
				if (p->listitems[i] != NULL) {
					chHeapFree (p->listitems[i]);
					p->listitems[i] = NULL;
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
				if (bleL2CapSend (p->txbuf) != NRF_SUCCESS) {
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

orchard_app("Radio Chat", "chat.rgb", 0,
	chat_init, chat_start, chat_event, chat_exit, 9999);
