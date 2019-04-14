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

#include "orchard-app.h"
#include "orchard-ui.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_gatts_lld.h"
#include "ble_peer.h"

#include "i2s_lld.h"
#include "fontlist.h"

#include <stdio.h>
#include <string.h>

static OrchardAppRadioEvent radio_evt;
static char buf[128];

static void notify_handler(void * arg)
{
	OrchardAppRadioEvent * evt;
	ble_gatts_evt_rw_authorize_request_t * req;

	evt = arg;

	req = &evt->evt.evt.gatts_evt.params.authorize_request;
	if (req->request.write.handle != ch_handle.value_handle)
		return;

	memcpy (&radio_evt, evt, sizeof (radio_evt));

	orchardAppRun (orchardAppByName("Radio notification"));

	return;
}

static uint32_t notify_init(OrchardAppContext *context)
{
	(void)context;

	/* This should only happen for auto-init */

	if (context == NULL) {
		app_radio_notify = notify_handler;
	}

	return (0);
}

static void notify_start(OrchardAppContext *context)
{
	font_t font;
	ble_peer_entry * peer;
	ble_gatts_evt_rw_authorize_request_t * req;

	(void)context;

	gdispClear (Black);

	peer = blePeerFind (ble_peer_addr.addr);

	if (peer == NULL)
		snprintf (buf, 128, "Badge %X:%X:%X:%X:%X:%X",
		    ble_peer_addr.addr[5],  ble_peer_addr.addr[4],
		    ble_peer_addr.addr[3],  ble_peer_addr.addr[2],
		    ble_peer_addr.addr[1],  ble_peer_addr.addr[0]);
	else
		snprintf (buf, 128, "Badge %s", peer->ble_peer_name);

	font = gdispOpenFont(FONT_FIXED);

	gdispDrawStringBox (0, (gdispGetHeight() >> 1) -
	    gdispGetFontMetric(font, fontHeight),
	    gdispGetWidth(), gdispGetFontMetric(font, fontHeight),
	    buf, font, White, justifyCenter);

	req = &radio_evt.evt.evt.gatts_evt.params.authorize_request;

	if (req->request.write.handle == ch_handle.value_handle) {
		gdispDrawStringBox (0, (gdispGetHeight() >> 1), 
		    gdispGetWidth(), gdispGetFontMetric(font, fontHeight),
		    "WANTS TO CHAT", font, Green, justifyCenter);
	}

	gdispCloseFont (font);	
	i2sPlay("alert1.snd");

	chThdSleepMilliseconds (5000);

	orchardAppExit ();

	return;
}

void notify_event(OrchardAppContext *context, const OrchardAppEvent *event)
{
	(void)context;
	(void)event;

	return;
}

static void notify_exit(OrchardAppContext *context)
{
	(void)context;

	return;
}

orchard_app("Radio notification", NULL, APP_FLAG_HIDDEN|APP_FLAG_AUTOINIT,
	notify_init, notify_start, notify_event, notify_exit, 9999);
