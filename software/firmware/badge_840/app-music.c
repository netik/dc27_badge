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

#include "badge.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#define MUSIC_SAMPLES 2048
#define MUSIC_BYTES (MUSIC_SAMPLES * 2)
#define MUSIC_FFT_MAX_AMPLITUDE 128

#define BACKGROUND HTML2COLOR(0x470b67)


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

	f_opendir (&d, "\\");

	i = 0;

	while (1) {
		if (f_readdir (&d, &info) != FR_OK || info.fname[0] == 0)
			break;
		if (strstr (info.fname, ".RAW") != NULL)
			i++;
	}

	f_closedir (&d);

	if (i == 0) {
		orchardAppExit ();
		return;
	}

	i += 2;

	p = chHeapAlloc (NULL, sizeof(MusicHandles));
	memset (p, 0, sizeof(MusicHandles));
	p->listitems = chHeapAlloc (NULL, sizeof(char *) * i);
	p->itemcnt = i;

	p->listitems[0] = "Choose a song";
	p->listitems[1] = "Exit";

	f_opendir (&d, "\\");

	i = 2;

	while (1) {
		if (f_readdir (&d, &info) != FR_OK || info.fname[0] == 0)
			break;
		if (strstr (info.fname, ".RAW") != NULL) {
			p->listitems[i] =
			    chHeapAlloc (NULL, strlen (info.fname) + 1);
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
	 * by 12. This seems to yield a reasonable range.
	 */

	for (i = 0; i < MUSIC_SAMPLES / 4; i++) {
		sum = ((p->real[i] * p->real[i]) +
		    (p->imaginary[i] * p->imaginary[i]));
		p->real[i] = (short)(sqrt (sum) / 14.0);
	}

	/*
	 * Draw the bar graph. We draw 102 bars. Each bar contains 5 
	 * adjacent bins averaged together, which ads up to 510 bins.
	 * At 2 pixels per bar, plus 1 pixel between for spacing, this
	 * works out to 306 pixels, which roughly fits the 320 pixel
	 * horizontal display resolution with 14 pixels left over.
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

	i2sPlay (NULL);

	/* Initialize some of the display write window info. */

	GDISP->p.y = 240 - MUSIC_FFT_MAX_AMPLITUDE;
	GDISP->p.cx = 1;
	GDISP->p.cy = MUSIC_FFT_MAX_AMPLITUDE;

	if (f_open (&f, fname, FA_READ) != FR_OK)
		return (0);

	i2sBuf = chHeapAlloc (NULL,
	    (MUSIC_SAMPLES * sizeof(uint16_t)) * 2);

	buf = i2sBuf;
	p1 = buf;
	p2 = buf + MUSIC_SAMPLES;

	f_read (&f, buf, MUSIC_BYTES, &br);

	gs = ginputGetMouse (0);
	geventListenerInit (&gl);
	geventAttachSource (&gl, gs, GLISTEN_MOUSEMETA);

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

	f_close (&f);
	chHeapFree (i2sBuf);

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

		putImageFile ("MUSIC.RGB", 0, 0);

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

		if (musicPlay (p, p->listitems[uiContext->selected + 1]) != 0)
			i2sPlay ("click.raw");

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
		chHeapFree (p->listitems[i]);

	chHeapFree (p->listitems);
	chHeapFree (p);

	context->priv = NULL;

	return;
}

orchard_app("Play Music", "mplay.rgb", 0, music_init, music_start,
    music_event, music_exit, 9999);
