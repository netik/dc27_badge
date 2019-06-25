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

#include "fontlist.h"

#include "src/gdisp/gdisp_driver.h"

#include "ble_lld.h"
#include "nrf52radio_lld.h"
#include "nrf52i2s_lld.h"

#include "userconfig.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct spectrum_state {
	uint8_t		airplane;
	GListener	gl;
} SpectrumState;

static void
paramShow (uint32_t spanHz, uint32_t centerHz)
{
	font_t font;
	char str[64];
	coord_t width;

	width = gdispGetWidth();

	gdispGFillArea (GDISP, 40, 0, width - 80, 80, Teal);

	font = gdispOpenFont (FONT_KEYBOARD);

	snprintf (str, sizeof(str), "Start: %dMHz", 2360);
  
	gdispDrawStringBox (0, 0, gdispGetWidth(),
	    gdispGetFontMetric(font, fontHeight),
	    str, font, White, justifyCenter);

	snprintf (str, sizeof(str), "Center: %ldMHz", centerHz);
  
	gdispDrawStringBox (0, gdispGetFontMetric(font, fontHeight),
	    width, gdispGetFontMetric(font, fontHeight),
	    str, font, White, justifyCenter);

	snprintf (str, sizeof(str), "End: %dMHz", 2499);
  
	gdispDrawStringBox (0, gdispGetFontMetric(font, fontHeight) * 2,
	    width, gdispGetFontMetric(font, fontHeight),
	    str, font, White, justifyCenter);

	snprintf (str, sizeof(str), "Span: %ldMHz", spanHz);
  
	gdispDrawStringBox (0, gdispGetFontMetric(font, fontHeight) * 3,
	    width, gdispGetFontMetric(font, fontHeight),
	    str, font, White, justifyCenter);

	gdispCloseFont (font);

	return;
}

static uint32_t
spectrum_init (OrchardAppContext *context)
{
	(void)context;

	/*
	 * We don't want any extra stack space allocated for us.
	 * We'll use the heap.
	 */

	return (0);
}

static void
spectrum_start (OrchardAppContext *context)
{
	SpectrumState * state;
	GSourceHandle gs;
	userconfig * config;

	gdispClear (Teal);

	state = malloc (sizeof(SpectrumState));
	context->priv = state;

	paramShow (140, 2430);

	gs = ginputGetMouse (0);
	geventListenerInit (&state->gl);
	geventAttachSource (&state->gl, gs, GLISTEN_MOUSEMETA);
	geventRegisterCallback (&state->gl,
	    orchardAppUgfxCallback, &state->gl);

	orchardAppTimer (context, 1000, FALSE);

	config = getConfig ();
	state->airplane = config->airplane_mode;
	config->airplane_mode = 1;

	bleDisable ();
	nrf52radioStart ();

	return;
}

static void
spectrum_event (OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	int8_t rssi;
	int i, j, x;

	if (event->type == appEvent && event->app.event == appStart)
		orchardAppTimer (context, 1, TRUE);

	if (event->type == ugfxEvent) {
		i2sPlay ("sound/click.snd");
		orchardAppExit ();
		return;
	}

	if (event->type == timerEvent) {
		GDISP->p.y = 112;
		GDISP->p.cx = 2;
		GDISP->p.cy = gdispGetHeight () - 112;
		for (j = 0; j < 140; j++) {
			nrf52radioChanSet (2360 + j);
			nrf52radioRxEnable ();
			nrf52radioRssiGet (&rssi);
			nrf52radioRxDisable ();
			GDISP->p.x = 20 + (j * 2);
			gdisp_lld_write_start (GDISP);
			for (i = 0; i < 128; i++) {
				if (rssi == i)
					GDISP->p.color = Yellow;
				else if (rssi > i)
					GDISP->p.color = Teal;
				else
					GDISP->p.color = Navy;
				for (x = 0; x < 2; x++)
					gdisp_lld_write_color (GDISP);
			}
			gdisp_lld_write_stop (GDISP);
		}
	}

	return;
}

static void
spectrum_exit (OrchardAppContext *context)
{
	SpectrumState * state;
	userconfig * config;

	state = context->priv;

	nrf52radioStop ();
	bleEnable ();

	config = getConfig ();
	config->airplane_mode = state->airplane;

       	geventDetachSource (&state->gl, NULL);
	geventRegisterCallback (&state->gl, NULL, NULL);
	free (state);

	return;
}

orchard_app("RF Spectrum", "icons/mask.rgb", 0, spectrum_init,
    spectrum_start, spectrum_event, spectrum_exit, 9999);
