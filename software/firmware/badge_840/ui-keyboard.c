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

#include "fontlist.h"
#include "ides_gfx.h"
#include "src/gdisp/gdisp_driver.h"
#include "src/gwin/gwin_class.h"

/*
 * We need two widgets: a console and the keyboard.
 * We also need one listener object for detecting keypresses
 */

typedef struct _KeyboardHandles {
	GListener	gl;
	GHandle		ghConsole;
	GHandle		ghKeyboard;
	font_t		font;
	uint8_t		pos;
} KeyboardHandles;

/*
 * The console widget does not implement backspace, so we have to
 * do it here. Note: this function is only compatible with the
 * console widget if history support is turned off. When it's on, the
 * console widget maintains an internal buffer of the screen contents
 * which we can't erase. (We build the console widget with history
 * support disabled.)
 */

static void backspace (KeyboardHandles * p)
{
	GConsoleObject * cons;
	uint8_t w;
	uint8_t h;

	h = gdispGetFontMetric (p->font, fontHeight);
	w = gdispGetFontMetric (p->font, fontMaxWidth);
	cons = (GConsoleObject *)p->ghConsole;

	/* Adjust the cursor position */

	if (cons->cx == 0) {
		cons->cy -= h;
		cons->cx = gdispGetWidth() - (w * 2);
	} else
		cons->cx -= w;

	_gwinDrawStart (p->ghConsole);

	/* Clear the clip variables otherwise gdispGFillArea() might fail. */

	p->ghConsole->display->clipx0 = 0;
	p->ghConsole->display->clipy0 = 0;
	p->ghConsole->display->clipx1 = gdispGetWidth ();
	p->ghConsole->display->clipy1 = gdispGetHeight ();

	_gwinDrawEnd (p->ghConsole);

	/*
	 * Black out the character under the cursor. uGFX does not
	 * actually draw anything when you ask it to render a space,
	 * so we have to do this manually.
	 */

	gdispGFillArea (p->ghConsole->display, cons->cx, cons->cy,
           w, h, cons->g.bgcolor);

	return;
}

static uint8_t handle_input (char * name, uint8_t max,
	GEventKeyboard * pk, KeyboardHandles * p)
{
	uint8_t r;
	
	if (pk->bytecount == 0)
		return (0);

	switch (pk->c[0]) {
	case GKEY_BACKSPACE:
		if (p->pos != 0) {
			p->pos--;
			name[p->pos] = 0x0;
			backspace (p);
			backspace (p);
			gwinPutChar (p->ghConsole, '_');
		}
		r = 0;
		break;
	case GKEY_ENTER:
		r = 1;
		backspace (p);
		gwinPrintf (p->ghConsole, "\n");
		break;
	case '@':
		r = 0xFF;
		break;
	default:
		if (p->pos == max) {
			/*playDoh ();*/
		} else {
			name[p->pos] = pk->c[0];
			p->pos++;
			backspace (p);
			gwinPrintf (p->ghConsole, "%c_", pk->c[0]);
		}
		r = 0;
		break;
	}

	return (r);
}

static void keyboard_start (OrchardAppContext *context)
{
	KeyboardHandles * p;
	OrchardUiContext * ctx;
	GWidgetInit wi;

	ctx = context->instance->uicontext;
	ctx->priv = chHeapAlloc (NULL, sizeof(KeyboardHandles));
	p = ctx->priv;

	p->font = gdispOpenFont (FONT_KEYBOARD);
	gwinSetDefaultFont (p->font);

	/* Draw the console/text entry widget */

	gwinWidgetClearInit (&wi);
	wi.g.show = FALSE;
	wi.g.x = 0;
	wi.g.y = 0;
	wi.g.width = gdispGetWidth();
	wi.g.height = gdispGetHeight() >> 1;
	p->ghConsole = gwinConsoleCreate (0, &wi.g);
	gwinSetColor (p->ghConsole, White);
	gwinSetBgColor (p->ghConsole, Black);
	gwinShow (p->ghConsole);
	gwinClear (p->ghConsole);
	gwinPrintf (p->ghConsole, "%s\n\n",
		context->instance->uicontext->itemlist[0]);

	/* Draw the keyboard widget */
	wi.g.y = gdispGetHeight() >> 1;
	p->ghKeyboard = gwinKeyboardCreate (0, &wi);
	gwinSetStyle (p->ghKeyboard, &DarkPurpleFilledStyle);
	gwinShow (p->ghKeyboard);

	/* Wait for events */
	geventListenerInit (&p->gl);
	gwinAttachListener (&p->gl);

	geventAttachSource (&p->gl, gwinKeyboardGetEventSource (p->ghKeyboard),
		GLISTEN_KEYTRANSITIONS | GLISTEN_KEYUP);

	geventRegisterCallback (&p->gl, orchardAppUgfxCallback, &p->gl);

	gwinPrintf (p->ghConsole, "%s_", ctx->itemlist[1]);
	p->pos = strlen (ctx->itemlist[1]);

	return;
}

static void keyboard_event(OrchardAppContext *context,
	const OrchardAppEvent *event)
{
 	GEventKeyboard * pk;
	GConsoleObject * cons;
	KeyboardHandles * p;
	GEvent * pe;
	char * name;
	uint8_t max;
	uint8_t ret;

	p = context->instance->uicontext->priv;

	if (event->type == uiEvent) {
		cons = (GConsoleObject *)p->ghConsole;
		gdispGFillArea (p->ghConsole->display, 0, 0,
		    gdispGetWidth (), gdispGetHeight () / 2, cons->g.bgcolor);
		cons->cx = 0;
		cons->cy = 0;
		gwinPrintf (p->ghConsole, "%s\n\n",
	    		context->instance->uicontext->itemlist[0]);
		if (event->ui.flags == uiCancel)
			p->pos = 0;
		gwinPrintf (p->ghConsole, "%s_",
		    context->instance->uicontext->itemlist[1]);
	}

	if (event->type != ugfxEvent)
		return;

	pe = event->ugfx.pEvent;

	if (pe->type == GEVENT_KEYBOARD)
		pk = (GEventKeyboard *)pe;
	else
		return;

	name = (char *)context->instance->uicontext->itemlist[1];
	max = context->instance->uicontext->total;

	ret = handle_input (name, max, pk, p);

	if (ret != 0) {
		context->instance->uicontext->total = ret;
		context->instance->uicontext->selected = p->pos;
		chEvtBroadcast (&ui_completed);
	}

	return;  
}

static void keyboard_exit(OrchardAppContext *context)
{
	KeyboardHandles * p;

	p = context->instance->uicontext->priv;

	geventRegisterCallback (&p->gl, NULL, NULL);
	geventDetachSource (&p->gl, NULL);
	gwinDestroy (p->ghConsole);
	gwinDestroy (p->ghKeyboard);

	gdispCloseFont (p->font);

	chHeapFree (p);
	context->instance->uicontext->priv = NULL;

	return;
}

orchard_ui("keyboard", keyboard_start, keyboard_event, keyboard_exit);

