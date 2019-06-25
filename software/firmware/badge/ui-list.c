/*-
 * Copyright (c) 2016
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
#include "orchard-events.h"
#include "orchard-ui.h"

#include <string.h>
#include <stdlib.h>

#include "nrf52i2s_lld.h"
#include "fontlist.h"

typedef struct _ListHandles {
	GListener	gl;
	GHandle		ghList;
	font_t		font;
} ListHandles;

static void list_start (OrchardAppContext *context)
{
	ListHandles * p;
	OrchardUiContext * ctx;
	GWidgetInit wi;
	unsigned int i;

	ctx = context->instance->uicontext;
	ctx->priv = malloc (sizeof(ListHandles));
	p = ctx->priv;

	p->font = gdispOpenFont (FONT_FIXED);
	gwinSetDefaultFont (p->font);

        gdispFillArea (0, 0, gdispGetWidth(),
	    gdispGetFontMetric(p->font, fontHeight), Blue);

	gdispDrawStringBox (0, 0, gdispGetWidth(),
	    gdispGetFontMetric(p->font, fontHeight),
	    ctx->itemlist[0],
	    p->font, White, justifyCenter);

	/* Draw the console/text entry widget */

	gwinWidgetClearInit (&wi);
	wi.g.show = TRUE;
	wi.g.x = 0;
	wi.g.y = gdispGetFontMetric (p->font, fontHeight);
	wi.g.width = gdispGetWidth();
	wi.g.height = gdispGetHeight() -
	    gdispGetFontMetric (p->font, fontHeight);
	p->ghList = gwinListCreate (0, &wi, FALSE);

	/* Add users to the list. */

	for (i = 0; i < ctx->total; i++)
		gwinListAddItem (p->ghList, ctx->itemlist[i + 1], FALSE);

	/* Wait for events */
	geventListenerInit (&p->gl);
	gwinAttachListener (&p->gl);

	geventRegisterCallback (&p->gl, orchardAppUgfxCallback, &p->gl);

	return;
}

static void list_event(OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	ListHandles * p;
	GEvent * pe;
	GEventGWinList * ple;
	OrchardUiContext * ctx;
	int16_t selected;

	ctx = context->instance->uicontext;
	p = ctx->priv;

        if (event->type == uiEvent) {
		/* Add a new item */
		if (event->ui.flags == uiOK) {
			gwinListAddItem (p->ghList,
			    ctx->itemlist[ctx->selected], FALSE);
		}
	}

	/* handle joypad events */
	if (event->type == keyEvent && event->key.flags == keyPress) {
		if (event->key.code == keyADown ||
		    event->key.code == keyBDown) {
			selected = gwinListGetSelected (p->ghList);
			if (selected < ((GListObject *)p->ghList)->cnt) 
				selected++;
			gwinListSetSelected (p->ghList, selected ,TRUE);
			gwinListViewItem (p->ghList, selected);
			i2sPlay ("sound/click.snd");
		}
		if (event->key.code == keyAUp ||
		    event->key.code == keyBUp) {
			selected = gwinListGetSelected (p->ghList);
			if (selected > 0)
				selected--;
			gwinListSetSelected (p->ghList, selected ,TRUE);
			gwinListViewItem (p->ghList, selected);
			i2sPlay ("sound/click.snd");
		}
		if (event->key.code == keyASelect ||
		    event->key.code == keyBSelect) {
			ctx->selected = gwinListGetSelected (p->ghList);
			i2sPlay ("sound/click.snd");
			chEvtBroadcast (&ui_completed);
		}
	}

	if (event->type != ugfxEvent)
		return;

	pe = event->ugfx.pEvent;

	if (pe->type == GEVENT_GWIN_LIST) {
		ple = (GEventGWinList *)pe;
		i2sPlay ("sound/click.snd");
		ctx->selected = ple->item;
		chEvtBroadcast (&ui_completed);
	}

	return;  
}

static void list_exit(OrchardAppContext *context)
{
	ListHandles * p;

	p = context->instance->uicontext->priv;

	geventRegisterCallback (&p->gl, NULL, NULL);
	geventDetachSource (&p->gl, NULL);
	gwinDestroy (p->ghList);

	gdispCloseFont (p->font);

	gdispClear (Black);

	free (p);
	context->instance->uicontext->priv = NULL;

	return;
}

orchard_ui("list", list_start, list_event, list_exit);

