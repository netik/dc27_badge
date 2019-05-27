/*-
 * Copyright (c) 2016, 2019
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
#include <string.h>

#include "orchard-app.h"
#include "orchard-ui.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_gattc_lld.h"
#include "ble_gatts_lld.h"
#include "ble_peer.h"

#include "i2s_lld.h"
#include "fontlist.h"
#include "ides_gfx.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"

static OrchardAppRadioEvent radio_evt;
static char buf[128];

typedef struct _NHandles {
	GListener gl;
	GHandle	ghACCEPT;
	GHandle	ghDECLINE;
	font_t	font;
	char * app;
	uint8_t response_accept;
	uint8_t response_decline;
	uint8_t handle;
} NotifyHandles;

static bool notify_handler(void * arg)
{
	OrchardAppRadioEvent * evt;
        ble_gatts_evt_rw_authorize_request_t * rw;
	ble_gatts_evt_write_t * req;

	evt = arg;

	rw = &evt->evt.evt.gatts_evt.params.authorize_request;
	req = &rw->request.write;

	if (rw->request.write.handle != ch_handle.value_handle &&
	    req->data[0] != BLE_IDES_CHATREQ_REQUEST)
		return (FALSE);

	if (rw->request.write.handle != gm_handle.value_handle &&
	    req->data[0] != BLE_IDES_GAMEATTACK_CHALLENGE)
		return (FALSE);

	memcpy (&radio_evt, evt, sizeof (radio_evt));

	orchardAppRun (orchardAppByName ("Radio notification"));

	return (TRUE);
}

static uint32_t notify_init(OrchardAppContext *context)
{
	(void)context;

	/* This should only happen for auto-init */

	if (context == NULL)
		app_radio_notify = notify_handler;

	return (0);
}

static void notify_start(OrchardAppContext *context)
{
	ble_peer_entry * peer;
	ble_gatts_evt_rw_authorize_request_t * rw;
	NotifyHandles * p;
	GWidgetInit wi;

	p = malloc (sizeof (NotifyHandles));

	if (p == NULL) {
		orchardAppExit ();
		return;
	}

	context->priv = p;

	putImageFile ("images/undrattk.rgb", 0, 0);
	i2sPlay("sound/klaxon.snd");

	peer = blePeerFind (ble_peer_addr.addr);

	if (peer == NULL)
		snprintf (buf, 128, "Badge %X:%X:%X:%X:%X:%X",
		    ble_peer_addr.addr[5],  ble_peer_addr.addr[4],
		    ble_peer_addr.addr[3],  ble_peer_addr.addr[2],
		    ble_peer_addr.addr[1],  ble_peer_addr.addr[0]);
	else
		snprintf (buf, 128, "Badge %s", peer->ble_peer_name);

	p->font = gdispOpenFont(FONT_FIXED);

	gdispDrawStringBox (0, 50 -
	    gdispGetFontMetric(p->font, fontHeight),
	    gdispGetWidth(), gdispGetFontMetric(p->font, fontHeight),
	    buf, p->font, White, justifyCenter);

	rw = &radio_evt.evt.evt.gatts_evt.params.authorize_request;

	if (rw->request.write.handle == ch_handle.value_handle) {
		gdispDrawStringBox (0, 50, 
		    gdispGetWidth(), gdispGetFontMetric(p->font, fontHeight),
		    "WANTS TO CHAT", p->font, White, justifyCenter);
		p->handle = ch_handle.value_handle;
		p->app = "Radio Chat";
		p->response_accept = BLE_IDES_CHATREQ_ACCEPT;
		p->response_decline = BLE_IDES_CHATREQ_DECLINE;
	}

	if (rw->request.write.handle == gm_handle.value_handle) {
		gdispDrawStringBox (0, 50, 
		    gdispGetWidth(), gdispGetFontMetric(p->font, fontHeight),
		    "IS CHALLENGING YOU", p->font, White, justifyCenter);
		p->handle = gm_handle.value_handle;
		p->app = "Sea Battle";
		p->response_accept = BLE_IDES_GAMEATTACK_ACCEPT;
		p->response_decline = BLE_IDES_GAMEATTACK_DECLINE;
	}

	gwinSetDefaultStyle (&RedButtonStyle, FALSE);
	gwinSetDefaultFont (p->font);
	gwinWidgetClearInit (&wi);
	wi.g.show = TRUE;
	wi.g.x = 0;
	wi.g.y = 210;
	wi.g.width = 150;
	wi.g.height = 30;
	wi.text = "DECLINE";
	p->ghDECLINE = gwinButtonCreate(0, &wi);

	wi.g.x = 170;
	wi.text = "ACCEPT";
	p->ghACCEPT = gwinButtonCreate(0, &wi);

	gwinSetDefaultStyle (&BlackWidgetStyle, FALSE);

	geventListenerInit (&p->gl);
	gwinAttachListener (&p->gl);
	geventRegisterCallback (&p->gl, orchardAppUgfxCallback, &p->gl);

	return;
}

void notify_event(OrchardAppContext *context, const OrchardAppEvent *event)
{
	GEvent * pe;
	GEventGWinButton * be;
	NotifyHandles * p;

	p = context->priv;

	if (event->type == ugfxEvent) {
		pe = event->ugfx.pEvent;
		if (pe->type == GEVENT_GWIN_BUTTON) {
			be = (GEventGWinButton *)pe;
			if (be->gwin == p->ghACCEPT) {
				bleGattcWrite (p->handle,
				    (uint8_t *)&p->response_accept, 1, TRUE);
				orchardAppRun (orchardAppByName(p->app));
			}
			if (be->gwin == p->ghDECLINE) {
				bleGattcWrite (p->handle,
				    (uint8_t *)&p->response_decline, 1, TRUE);
				bleGapDisconnect ();
				orchardAppExit ();
			}
		}
	}

	return;
}

static void notify_exit(OrchardAppContext *context)
{
	NotifyHandles * p;

	p = context->priv;
	context->priv = NULL;

	gdispCloseFont (p->font);
	gwinDestroy (p->ghACCEPT);
	gwinDestroy (p->ghDECLINE);

	geventDetachSource (&p->gl, NULL);
	geventRegisterCallback (&p->gl, NULL, NULL);

	free (p);

	return;
}

orchard_app("Radio notification", NULL, APP_FLAG_HIDDEN|APP_FLAG_AUTOINIT,
	notify_init, notify_start, notify_event, notify_exit, 9999);
