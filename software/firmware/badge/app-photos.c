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

#include "orchard-app.h"
#include "orchard-ui.h"

#include "scroll_lld.h"
#include "i2s_lld.h"

#include "ff.h"
#include "ffconf.h"
#include "ides_gfx.h"

#include <string.h>
#include <stdio.h>

typedef struct _PhotoHandles {
	DIR		d;
	int		cnt;
	GListener	gl;
} PhotoHandles;

static uint32_t
photos_init (OrchardAppContext *context)
{
	(void)context;

	/*
	 * We don't want any extra stack space allocated for us.
	 * We'll use the heap.
	 */

	return (0);
}

static void
photos_start (OrchardAppContext *context)
{
	PhotoHandles * p;

	context->priv = malloc(sizeof(PhotoHandles));
	memset (context->priv, 0, sizeof(PhotoHandles));

	p = context->priv;

	geventListenerInit (&p->gl);
	geventRegisterCallback (&p->gl, orchardAppUgfxCallback, &p->gl);

	return;
}

static void
photos_event (OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	PhotoHandles * p;
	FILINFO info;
	GEventMouse * me;
	GSourceHandle gs;
	char str[64];

	p = context->priv;

	if (event->type == ugfxEvent) {
	        me = (GEventMouse *)event->ugfx.pEvent;
		if (me->buttons & GMETA_MOUSE_DOWN) {
			i2sPlay ("sound/click.snd");
			orchardAppExit ();
			return;
		}
	}

	if (event->type == appEvent && event->app.event == appStart) {
          if (f_opendir (&p->d, "photos") != FR_OK) {
            /* give a helpful message if no photo dir. */
            putImageFile("images/nophotos.rgb", 0, 0);
            gs = ginputGetMouse (0);
            geventAttachSource (&p->gl, gs, GLISTEN_MOUSEMETA);
            orchardAppTimer (context, 5000000, FALSE);
          } else {
            orchardAppTimer (context, 1000, FALSE);
          }
	}

	if (event->type == timerEvent) {
		memset (&info, 0, sizeof(info));
		if (f_readdir (&p->d, &info) != FR_OK ||
		    info.fname[0] == 0) {
			if (p->cnt == 0) {
				orchardAppExit ();
				return;
			}
			p->cnt = 0;
			f_closedir (&p->d);
			f_opendir (&p->d, "photos");
			orchardAppTimer (context, 1, FALSE);
		} else {
			if (strstr (info.fname, ".RGB") == NULL) {
				orchardAppExit ();
				return;
			}
			p->cnt++;
			snprintf (str, sizeof(str), "photos/%s", info.fname);
			geventDetachSource (&p->gl, NULL);
			if (scrollImage (str, 1) == -1) {
				i2sPlay ("sound/click.snd");
				orchardAppExit ();
				return;
			}
			geventRegisterCallback (&p->gl,
			    orchardAppUgfxCallback, &p->gl);
			gs = ginputGetMouse (0);
        		geventAttachSource (&p->gl, gs, GLISTEN_MOUSEMETA);
			orchardAppTimer (context, 5000000, FALSE);
		}
	}

	if (event->type == appEvent && event->app.event == appTerminate) {
		f_closedir (&p->d);
	}

	return;
}

static void
photos_exit (OrchardAppContext *context)
{
	PhotoHandles * p;

	scrollCount (0);

	p = context->priv;

	geventRegisterCallback (&p->gl, NULL, NULL);
	geventDetachSource (&p->gl, NULL);

	free (p);

	return;
}

orchard_app("Photos", "icons/helmet.rgb", 0, photos_init,
    photos_start, photos_event, photos_exit, 9999);
