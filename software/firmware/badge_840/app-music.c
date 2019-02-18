/*-
 * Copyright (c) 2017
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
#include "chprintf.h"
#include "orchard-app.h"
#include "orchard-ui.h"
#include "i2s_lld.h"

#include "ff.h"
#include "ffconf.h"

#ifdef notyet
#include "fix_fft.h"
#endif

#include "src/gdisp/gdisp_driver.h"

#include "fontlist.h"

#include <string.h>
#include <stdlib.h>

#define BACKGROUND HTML2COLOR(0x470b67)

typedef struct _MusicHandles {
	char **			listitems;
	int			itemcnt;
	OrchardUiContext	uiCtx;
	short			in[128];
	short			out[128];
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

#ifdef notyet
static void
columnDraw (int col, short amp, int erase)
{
	int x;
	int y;

	/* Clamp the amplitude at 128 pixels */

	if (amp > 128)
		amp = 128;

	/* Set up the drawing aperture */

	GDISP->p.x = col * 5;
	GDISP->p.y = 240 - amp;
	GDISP->p.cx = 4;
	GDISP->p.cy = amp;

	/* Now draw the column */

	gdisp_lld_write_start (GDISP);
	for (y = amp; y > 0; y--) {
		if (erase)
			GDISP->p.color = BACKGROUND;
		else {
			if (y >= 0 && y < 32)
				GDISP->p.color = Lime;
			if (y >= 32 && y < 64)
				GDISP->p.color = Yellow;
			if (y >= 64 && y <= 128)
				GDISP->p.color = Red;
		}
		for (x = 0; x < 4; x++) {
			gdisp_lld_write_color (GDISP);
		}
	}
	gdisp_lld_write_stop (GDISP);

	return;
}
#endif

static int
musicPlay (MusicHandles * p, char * fname)
{
	FIL f;
	uint16_t * buf;
	uint16_t * i2sBuf;
	UINT br;
	GEventMouse * me = NULL;
	GSourceHandle gs;
	GListener gl;
#ifdef notyet
	int i;
	short b;
#endif
	int r = 0;

	i2sPlay (NULL);

	if (f_open (&f, fname, FA_READ) != FR_OK)
		return (0);

	i2sBuf = chHeapAlloc (NULL,
	    (I2S_SAMPLES * sizeof(uint16_t)) * 2);

	buf = i2sBuf;

	f_read (&f, buf, I2S_BYTES, &br);

	gs = ginputGetMouse (0);
	geventListenerInit (&gl);
	geventAttachSource (&gl, gs, GLISTEN_MOUSEMETA);

	while (1) {

		i2sSamplesPlay (buf, br >> 1);

#ifdef notyet
		for (i = 1; i < 63; i++) {
			b = p->in[i];
			columnDraw (i, b, 1);
		}

		for (i = 0; i < 128; i++)
			p->in[i] = buf[i] << 2;

		fix_fft (p->in, p->out, 7, 0);

		for (i = 1; i < 63; i++) {
			b = abs(p->in[i]);
			b >>= 3;
			p->in[i] = b;
			columnDraw (i, b, 0);
		}
#endif
		if (buf == i2sBuf)
			buf += I2S_SAMPLES;
		else
			buf = i2sBuf;

		f_read (&f, buf, I2S_BYTES, &br);

		i2sSamplesWait ();

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
	char buf[32];
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
		gdispClear (BACKGROUND);

		font = gdispOpenFont (FONT_FIXED);
		chsnprintf (buf, sizeof(buf), "Now playing: %s",
		    p->listitems[uiContext->selected + 1]);
		gdispDrawStringBox (0,
		    gdispGetFontMetric(font, fontHeight) * 2,
		    gdispGetWidth(), gdispGetFontMetric(font, fontHeight),
		    buf, font, Cyan, justifyCenter);
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
