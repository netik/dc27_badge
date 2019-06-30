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

#include "ides_gfx.h"
#include "nrf52i2s_lld.h"
#include "nrf52radio_lld.h"
#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "fontlist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define GEIGER_PKTLEN		37

#define RX_ACTION_NONE		0x00
#define RX_ACTION_SENSE_UPDATE	0x01
#define RX_ACTION_EXIT		0x02

typedef struct geiger_handles {
	GListener	gl;
	GHandle		ghExit;
	GHandle		ghBadgesOnly;
	GHandle		ghUp;
	GHandle		ghDown;
	font_t		font;
	int8_t		rxThreshold;
	bool		badgesOnly;
	volatile int	rxAction;
	char *		click;
	int		clickLen;
	uint8_t		i2sEnable;
	uint8_t		rxpkt[GEIGER_PKTLEN + 3];
	thread_t *	pThread;
} GHandles;

static void
geigerScan (GHandles * p, uint16_t freq)
{
        ble_ides_game_state_t * s;
	int8_t rssi;
	uint8_t * d;
	uint8_t l;

	/* Select frequency */

	nrf52radioChanSet (freq);

	/* Check for a packet */

	if (nrf52radioRx (p->rxpkt, GEIGER_PKTLEN, &rssi, 5) != NRF_SUCCESS)
		return;

	rssi = 127 - rssi;

	/*
	 * When in "Badges Only" mode, we only respond to advertisement
	 * packets from other DC27 bombs. We look for those by checking
	 * for our vendor-specific game data block.
	 */

	if (p->badgesOnly == TRUE) {
		d = &p->rxpkt[3 + 6];
		l = p->rxpkt[1];
		if (bleGapAdvBlockFind (&d, &l,
		    BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA)
		    != NRF_SUCCESS)
			return;
                s = (ble_ides_game_state_t *)d;
                if (s->ble_ides_company_id != BLE_COMPANY_ID_IDES)
			return;
	}

	if (rssi >= p->rxThreshold)
		i2sSamplesPlay (p->click, p->clickLen);

	return;
}

static THD_WORKING_AREA(waGeigerThread, 512);
static THD_FUNCTION(geigerThread, arg)
{
	GHandles * p;

	p = arg;

	chRegSetThreadName ("GeigerEvent");

	bleDisable ();
	nrf52radioStart ();

	i2sAudioAmpCtl (I2S_AMP_ON);

	while (1) {
		if (p->rxAction == RX_ACTION_EXIT)
			break;

		p->rxAction = RX_ACTION_NONE;

		geigerScan (p, 2402);
		geigerScan (p, 2496);
		geigerScan (p, 2480);
	}

	i2sAudioAmpCtl (I2S_AMP_OFF);

	nrf52radioStop ();
	bleEnable ();

	chSysLock ();
	chThdExitS (MSG_OK);

	return;
}

static void
geigerSenseAdjust (GHandles * p)
{
	char s[32];

	drawProgressBar (10, 80, 180, 20, 127, p->rxThreshold, false, false);

	sprintf (s, "RX Threshold: %d", p->rxThreshold);
	gdispFillArea (10, 105, 180, 20, Black);
	gdispDrawStringBox (10, 105, 180, 20, s,
	    p->font, White, justifyCenter);    

	return;
}

static void
geigerUiInit (GHandles * p)
{
	GWidgetInit wi;

	gwinSetDefaultStyle (&RedButtonStyle, FALSE);

	gwinWidgetClearInit (&wi);
	wi.g.show = TRUE;
	wi.g.x = 235;
	wi.g.y = 80;
	wi.g.width = 80;
	wi.g.height = 25;
	wi.text = "EXIT";
	p->ghExit = gwinButtonCreate(0, &wi);

	gwinSetDefaultStyle (&GreenButtonStyle, FALSE);

	wi.g.y = 130;
	wi.text = "Badges Only";
	p->ghBadgesOnly = gwinButtonCreate(0, &wi);

	wi.g.x = 10;
	wi.g.y = 130;
	wi.g.width = 50;
	wi.g.height = 25;
	wi.text = "<-";
	p->ghDown = gwinButtonCreate(0, &wi);

	wi.g.x = 140;
	wi.g.y = 130;
	wi.text = "->";
	p->ghUp = gwinButtonCreate(0, &wi);

	gwinSetDefaultStyle (&BlackWidgetStyle, FALSE);

	geigerSenseAdjust (p);

	return;
}

static void
geigerUiDestroy (GHandles * p)
{
	gwinDestroy (p->ghExit);
	gwinDestroy (p->ghBadgesOnly);
	gwinDestroy (p->ghUp);
	gwinDestroy (p->ghDown);

	return;
}

static uint32_t
geiger_init (OrchardAppContext *context)
{
	(void)context;

	/*
	 * We don't want any extra stack space allocated for us.
	 * We'll use the heap.
	 */

	return (0);
}

static void
geiger_start (OrchardAppContext *context)
{
	GHandles * p;
	int fd;
	struct stat s;

	putImageFile ("/images/radioact.rgb", 0,0);

	p = malloc (sizeof (GHandles));
	memset (p, 0, sizeof (GHandles));

	context->priv = p;

	i2sPlay (NULL);
	p->i2sEnable = i2sEnabled;
	i2sEnabled = FALSE;

	p->font = gdispOpenFont (FONT_XS);

	/* Load the click sound effect into RAM */

	fd = open ("/sound/geiger1.snd", O_RDONLY);
	fstat (fd, &s);
	p->clickLen = s.st_size / 2;
	p->click = malloc (s.st_size);
	read (fd, p->click, s.st_size);
	close (fd);

	p->rxThreshold = 40;
	p->rxAction = RX_ACTION_NONE;
	p->badgesOnly = FALSE;

	geigerUiInit (p);

	geventListenerInit (&p->gl);
        gwinAttachListener(&p->gl);
	geventRegisterCallback (&p->gl, orchardAppUgfxCallback, &p->gl);

	/* Launch the receiver thread; */

	p->pThread = chThdCreateStatic (waGeigerThread, sizeof(waGeigerThread),
            NORMALPRIO - 10, geigerThread, p);

	return;
}

static void
geiger_event (OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	GEvent * pe;
	GEventGWinButton * be;
	GHandles * p;

	p = context->priv;

	if (event->type == ugfxEvent) {
		pe = event->ugfx.pEvent;
		if (pe->type == GEVENT_GWIN_BUTTON) {
			be = (GEventGWinButton *)pe;
			if (be->gwin == p->ghExit) {
				orchardAppExit ();
				return;
			}
			if (be->gwin == p->ghBadgesOnly) {
				if (p->badgesOnly == FALSE)
					p->badgesOnly = TRUE;
				else
					p->badgesOnly = FALSE;
			}
			if (be->gwin == p->ghUp) {
				if (p->rxThreshold < 127) {
					p->rxThreshold++;
					geigerSenseAdjust (p);
				}
			}
			if (be->gwin == p->ghDown) {
				if (p->rxThreshold > 0 ) {
					p->rxThreshold--;
					geigerSenseAdjust (p);
				}
			}

			return;
		}
	}

	if (event->type == keyEvent && event->key.flags == keyPress) {
		if (event->key.code == keyADown ||
 		    event->key.code == keyBDown) {
			if (p->rxThreshold > 0 ) {
				p->rxThreshold--;
				geigerSenseAdjust (p);
			}
		}

		if (event->key.code == keyAUp ||
 		    event->key.code == keyBUp) {
			if (p->rxThreshold < 127) {
				p->rxThreshold++;
				geigerSenseAdjust (p);
			}
		}
	}

	return;
}

static void
geiger_exit (OrchardAppContext *context)
{
	GHandles * p;

	p = context->priv;

	p->rxAction = RX_ACTION_EXIT;
	chThdWait (p->pThread);

	i2sEnabled = p->i2sEnable;
	i2sPlay ("/sound/click.snd");

	geigerUiDestroy (p);

	geventDetachSource (&p->gl, NULL);
	geventRegisterCallback (&p->gl, NULL, NULL);

	gdispCloseFont (p->font);

	if (p->click != NULL)
		free (p->click);
	free (p);

	return;
}

orchard_app("RF Counter", "icons/danger.rgb",
    0, geiger_init, geiger_start, geiger_event, geiger_exit, 9999);
