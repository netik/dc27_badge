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

#include "userconfig.h"
#include "nrf52i2s_lld.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct _DoomHandles {
	GListener	gl;
	gdispImage 	img;
	int		idx;
} DoomHandles;

static uint32_t
doomguy_init (OrchardAppContext *context)
{
	(void)context;

	/*
	 * We don't want any extra stack space allocated for us.
	 * We'll use the heap.
	 */

	return (0);
}

static void
doomguy_start (OrchardAppContext *context)
{
	DoomHandles * p;
	GSourceHandle gs;
	userconfig * config;

	p = malloc (sizeof(DoomHandles));

	p->idx = 0;

	context->priv = p;

	gdispClear (HTML2COLOR(0xBAC5BA));

	config = getConfig ();

	if (config->rotate)
		gdispSetOrientation (GDISP_ROTATE_0);

	gs = ginputGetMouse (0);
	geventListenerInit (&p->gl);
	geventAttachSource (&p->gl, gs, GLISTEN_MOUSEMETA);
	geventRegisterCallback (&p->gl, orchardAppUgfxCallback, &p->gl);


	return;
}

static void
doomguy_event (OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	DoomHandles * p;
	GEventMouse * me;
	char name[32];
	int x, y;
	int r;

	p = context->priv;

	if (event->type == appEvent && event->app.event == appStart)
		orchardAppTimer (context, 1000, FALSE);

	if (event->type == timerEvent) {
		sprintf (name, "/doom/doom%04d.rgb", p->idx);

		r = gdispImageOpenFile (&p->img, name);
		if (r != GDISP_IMAGE_ERR_OK)
			p->idx = 0;
		else {
			x = (gdispGetWidth () - p->img.width) / 2;
			y = (gdispGetHeight () - p->img.height) / 2;
			gdispImageCache (&p->img);
			gdispImageDraw (&p->img, x, y,
			    p->img.width, p->img.height, 0, 0);
			gdispImageClose (&p->img);
			p->idx++;
		}

		orchardAppTimer (context, 600000, FALSE);
	}

	if (event->type == keyEvent && event->key.flags == keyPress) {
		i2sPlay ("sound/click.snd");
		orchardAppExit ();
		return;
	}

	if (event->type == ugfxEvent) {
		me = (GEventMouse *)event->ugfx.pEvent;
		if (me->buttons & GMETA_MOUSE_DOWN) {
			i2sPlay ("sound/click.snd");
			orchardAppExit ();
			return;
		}
	}

	return;
}

static void
doomguy_exit (OrchardAppContext *context)
{
	DoomHandles * p;

	p = context->priv;

	gdispSetOrientation (GDISP_DEFAULT_ORIENTATION);

	geventRegisterCallback (&p->gl, NULL, NULL);
	geventDetachSource (&p->gl, NULL);

	free (p);

	return;
}

orchard_app("Doomguy", "icons/doomguy.rgb",
    0, doomguy_init, doomguy_start, doomguy_event, doomguy_exit, 9999);
