/*-
 * Copyright (c) 2017-2019
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

#include "ch.h"
#include "hal.h"
#include "orchard-app.h"
#include "orchard-ui.h"
#include "i2s_lld.h"

#include "ff.h"
#include "ffconf.h"

#include "async_io_lld.h"

#include "fix_fft.h"

#include "src/gdisp/gdisp_driver.h"

#include "fontlist.h"

#include "ides_gfx.h"

#include "userconfig.h"
#include "led.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#define MUSIC_SAMPLES 2048
#define MUSIC_BYTES (MUSIC_SAMPLES * 2)
#define MUSIC_FFT_MAX_AMPLITUDE 128

#define BACKGROUND HTML2COLOR(0x470b67)
#define MUSICDIR "/sound"

typedef struct _MusicHandles {
	char **			listitems;
	int			itemcnt;
	OrchardUiContext	uiCtx;
	short			real[MUSIC_SAMPLES / 2];
	short			imaginary[MUSIC_SAMPLES / 2];
} MusicHandles;


static uint32_t
music_init(OrchardAppContext *context)
{
	(void)context;
	return (0);
}

static void
music_start (OrchardAppContext *context)
{
	MusicHandles * p;
	DIR d;
	FILINFO info;
	int i;

	f_opendir (&d, MUSICDIR);

	i = 0;

	while (1) {
		if (f_readdir (&d, &info) != FR_OK || info.fname[0] == 0)
			break;
		if (strstr (info.fname, ".SND") != NULL)
			i++;
	}

	f_closedir (&d);

	if (i == 0) {
		orchardAppExit ();
		return;
	}

	i += 2;

	p = malloc (sizeof(MusicHandles));
	memset (p, 0, sizeof(MusicHandles));
	p->listitems = malloc (sizeof(char *) * i);
	p->itemcnt = i;

	p->listitems[0] = "Choose a song";
	p->listitems[1] = "Exit";

	f_opendir (&d, MUSICDIR);

	i = 2;

	while (1) {
		if (f_readdir (&d, &info) != FR_OK || info.fname[0] == 0)
			break;
		if (strstr (info.fname, ".SND") != NULL) {
			p->listitems[i] =
			    malloc (strlen (info.fname) + 1);
			memset (p->listitems[i], 0, strlen (info.fname) + 1);
			strcpy (p->listitems[i], info.fname);
			i++;
		}
	}

	p->uiCtx.itemlist = (const char **)p->listitems;
	p->uiCtx.total = p->itemcnt - 1;

	context->instance->ui = getUiByName ("list");
	context->instance->uicontext = &p->uiCtx;
	context->instance->ui->start (context);

	context->priv = p;
	return;
}

static void
columnDraw (int col, short amp)
{
	int y;

	/* Clamp the amplitude at 128 pixels */

	if (amp > MUSIC_FFT_MAX_AMPLITUDE)
		amp = MUSIC_FFT_MAX_AMPLITUDE;

	/*
	 * Set the drawing aperture.
	 * We offset the bar graph by 7 pixels to the right
	 * so that it appears centered.
	 */

	GDISP->p.x = col;

	/* Now draw the column */

	gdisp_lld_write_start (GDISP);
	for (y = MUSIC_FFT_MAX_AMPLITUDE; y > 0; y--) {
		if (y > amp)
			GDISP->p.color = Black;
		else {
			if (y >= 0 && y < 32)
				GDISP->p.color = Lime;
			if (y >= 32 && y < 64)
				GDISP->p.color = Yellow;
			if (y >= 64 && y <= 128)
				GDISP->p.color = Red;
		}
		gdisp_lld_write_color (GDISP);
	}
	gdisp_lld_write_stop (GDISP);

	return;
}

static void
ledDraw (short amp)
{
	int i;

	if (amp > LED_COUNT_INTERNAL)
		amp = LED_COUNT_INTERNAL;

	for (i = LED_COUNT_INTERNAL; i >= 0; i--) {
		if (i > amp) {
			led_set (i - 1, 0, 0, 0);
		} else {
			if (i >= 1 && i <= 8)
				led_set (i - 1, 0, 255, 0);
			if (i >= 9 && i <= 16)
				led_set (i - 1, 255, 255, 0);
			if (i >= 17 && i <= 32)
				led_set (i - 1, 255, 0, 0);
		}
	}

	led_show ();
	
	return;
}

static void
musicVisualize (MusicHandles * p, uint16_t * samples)
{
	unsigned int sum;
	short b;
	int i;
	int r;

	/*
	 * Gather up one channel's worth of samples.
	 *
	 * Note: each sample is supposed to be a raw signed integer.
	 * Technically we're using the 2s complement values here. We
	 * could undo that, but experimentation shows that it doesn't
	 * seem to make any difference to the FFT function, so we
	 * save some CPU cycles here and just don't bother.
	 */

	for (i = 0; i < MUSIC_SAMPLES / 2; i++) {
		p->real[i] = samples[i * 2];
		p->imaginary[i] = 0;
	}

	/* Perform FFT calculation. 1<<10 is 1024, so m here is 10. */

	fix_fft (p->real, p->imaginary, 10, 0);

	/*
	 * Combine real and imaginary parts into absolute values. This
	 * is done by summing the square of the real portion and the
	 * square of the imaginary portion, then taking the square
	 * root.
	 *
	 * Note: we only use half the array in the output compared to
	 * the input. That is, we fed in 1024 samples, but we only use
	 * 512 output bins. It looks like the remaining 512 bins are
	 * just a mirror image of the first 512.
	 *
	 * We also apply some scaling by dividing the amplitude values
	 * by 14. This seems to yield a reasonable range.
	 */

	r = 0;

	for (i = 0; i < MUSIC_SAMPLES / 4; i++) {
		sum = ((p->real[i] * p->real[i]) +
		    (p->imaginary[i] * p->imaginary[i]));
		p->real[i] = (short)(sqrt (sum) / 14.0);
		r += p->real[i];
	}

	/*
	 * Take an average of the sample amplitudes and use it to
	 * frob the LEDs.
	 */

	r = r / (MUSIC_SAMPLES / 4);
	b = r;
	ledDraw (b);

	/*
	 * Draw the bar graph. We draw 160 bars that are each one pixel
	 * wide with one pixel of spacing in between. Each bar contains 3
	 * adjacent bins averaged together, which ads up to 480 bins.
	 * This leaves 32 bins left over. We start from the 16th bin
	 * so that the lowest and highest 16 bins are ones that are
	 * left undisplayed.
	 */

	r = 16;
	for (i = 0; i < 320; i += 2) {
		b = p->real[r];
		b += p->real[r + 1];
		b += p->real[r + 2];
		b /= 3;
		r += 3;
		columnDraw (i, b);
	}

	return;
}

static int
musicPlay (MusicHandles * p, char * fname)
{
	FIL f;
	uint16_t * buf;
	uint16_t * p1;
	uint16_t * p2;
	uint16_t * i2sBuf;
	UINT br;
	GEventMouse * me = NULL;
	GSourceHandle gs;
	GListener gl;
	int r = 0;
	uint8_t enb;
	userconfig *config;

	config = getConfig();

	i2sPlay (NULL);
	enb = i2sEnabled;
	i2sEnabled = FALSE;
        ledStop();

	/* Initialize some of the display write window info. */

	GDISP->p.y = 240 - MUSIC_FFT_MAX_AMPLITUDE;
	GDISP->p.cx = 1;
	GDISP->p.cy = MUSIC_FFT_MAX_AMPLITUDE;

	if (f_open (&f, fname, FA_READ) != FR_OK)
		return (0);

	i2sBuf = malloc ((MUSIC_SAMPLES * sizeof(uint16_t)) * 2);

	buf = i2sBuf;
	p1 = buf;
	p2 = buf + MUSIC_SAMPLES;

	f_read (&f, buf, MUSIC_BYTES, &br);

	gs = ginputGetMouse (0);
	geventListenerInit (&gl);
	geventAttachSource (&gl, gs, GLISTEN_MOUSEMETA);

	/* Power up the audio amp */

	i2sAudioAmpCtl (I2S_AMP_ON);

	while (1) {

		asyncIoRead (&f, p2, MUSIC_BYTES, &br);

		i2sSamplesPlay (p1, br >> 1);

		musicVisualize (p, p1);

		if (p1 == buf) {
 			p1 += MUSIC_SAMPLES;
			p2 = buf;
		} else {
			p1 = buf;
			p2 += MUSIC_SAMPLES;
		}

		i2sSamplesWait ();
		asyncIoWait ();

		if (br == 0)
			break;

		me = (GEventMouse *)geventEventWait (&gl, 0);
		if (me != NULL && me->type == GEVENT_TOUCH) {
			r = -1;
			break;
		}
	}

	/* Power down the audio amp */

	i2sAudioAmpCtl (I2S_AMP_ON);
	i2sEnabled = enb;

	ledSetPattern (config->led_pattern);

	f_close (&f);
	free (i2sBuf);

	geventDetachSource (&gl, NULL);

	return (r);
}

static void
music_event(OrchardAppContext *context, const OrchardAppEvent *event)
{
	OrchardUiContext * uiContext;
	const OrchardUi * ui;
	MusicHandles * p;
	font_t font;
	char musicfn[35];

	p = context->priv;
	ui = context->instance->ui;
	uiContext = context->instance->uicontext;

	if (event->type == appEvent && event->app.event == appTerminate) {
		if (ui != NULL)
 			ui->exit (context);
	}

	if (event->type == ugfxEvent || event->type == keyEvent)
		ui->event (context, event);

	if (event->type == uiEvent && event->ui.code == uiComplete &&
	    event->ui.flags == uiOK) {
		/*
		 * If this is the list ui exiting, it means we chose a
		 * song to play, or else the user selected exit.
		 */

		i2sWait ();
 		ui->exit (context);
		context->instance->ui = NULL;

		/* User chose the "EXIT" selection, bail out. */

		if (uiContext->selected == 0) {
			orchardAppExit ();
			return;
		}

		putImageFile ("images/music.rgb", 0, 0);

		font = gdispOpenFont (FONT_FIXED);

		gdispDrawStringBox (0,
		    gdispGetFontMetric(font, fontHeight) * 1,
		    gdispGetWidth () / 2,
		    gdispGetFontMetric(font, fontHeight),
		    "Now playing:", font, Cyan, justifyCenter);

		gdispDrawStringBox (0,
		    gdispGetFontMetric(font, fontHeight) * 2,
		    gdispGetWidth () / 2,
		    gdispGetFontMetric(font, fontHeight),
		    p->listitems[uiContext->selected + 1],
		    font, Cyan, justifyCenter);

		gdispCloseFont (font);

		chThdSetPriority (HIGHPRIO - 5);

		strcpy(musicfn, MUSICDIR);
    strcat(musicfn, "/");
    strcat(musicfn, p->listitems[uiContext->selected +1]);

		if (musicPlay (p, musicfn) != 0)
			i2sPlay ("sound/click.snd");

		chThdSetPriority (ORCHARD_APP_PRIO);

		orchardAppExit ();
	}

	return;
}

static void
music_exit(OrchardAppContext *context)
{
	MusicHandles * p;
	int i;

	p = context->priv;

	if (p == NULL)
            return;

	for (i = 2; i < p->itemcnt; i++)
		free (p->listitems[i]);

	free (p->listitems);
	free (p);

	context->priv = NULL;

	return;
}

orchard_app("Play Music", "icons/jelly.rgb", 0, music_init, music_start,
    music_event, music_exit, 9999);
