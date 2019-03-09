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

#include "chprintf.h"

#include "scroll_lld.h"
#include "i2s_lld.h"

#ifdef notyet
#include "userconfig.h"
#endif

#define MESSAGELEN	128

#include <string.h>

static uint32_t
ledsign_init(OrchardAppContext *context)
{
	(void)context;

	return (0);
}

static void
ledsign_start(OrchardAppContext *context)
{
	OrchardUiContext * keyboardUiContext;
#ifdef notyet
	userconfig * config;

	config = getConfig();
#endif
	keyboardUiContext = chHeapAlloc (NULL, sizeof(OrchardUiContext));
	keyboardUiContext->itemlist = (const char **)chHeapAlloc(NULL,
		sizeof(char *) * 2);
	keyboardUiContext->itemlist[0] =
		"Type a message,\npress ENTER when done.";
#ifdef notyet
	keyboardUiContext->itemlist[1] = config->led_string;
	keyboardUiContext->total = CONFIG_LEDSIGN_MAXLEN - 1;
#else
	context->priv = chHeapAlloc (NULL, MESSAGELEN);
	memset (context->priv,0, MESSAGELEN);
	keyboardUiContext->itemlist[1] = context->priv;
	keyboardUiContext->total = MESSAGELEN - 1;
#endif

	context->instance->ui = getUiByName("keyboard");
	context->instance->uicontext = keyboardUiContext;
	context->instance->ui->start (context);

	return;
}

static void
ledsign_event(OrchardAppContext *context, const OrchardAppEvent *event)
{
	uint8_t i;
	const OrchardUi * keyboardUi;
	OrchardUiContext * keyboardUiContext;
#ifdef notyet
	userconfig * config;
#endif
	const char * str;
	char fname[48];
	int sts = 1;

	keyboardUi = context->instance->ui;
	keyboardUiContext = context->instance->uicontext;

	if (event->type == ugfxEvent)
		keyboardUi->event (context, event);

 	if (event->type == uiEvent) {
		if (event->ui.code == uiComplete &&
		    event->ui.flags == uiOK) {
			keyboardUi->exit (context);
			context->instance->ui = NULL;
			gdispClear (Black);
#ifdef notyet
			config = getConfig();
			configSave (config);
#endif
			orchardAppTimer (context, 1, FALSE);
		}
	}

 	if (event->type == appEvent && event->app.event == appTerminate) {
		if (keyboardUi != NULL)
			keyboardUi->exit (context);
	}

        if (event->type == timerEvent) {
		scrollAreaSet (0, 0);
		str = keyboardUiContext->itemlist[1];
		do {
			for (i = 0; i < strlen (str); i++) {
				chsnprintf (fname, 47,
				    "font/led/%02X.rgb", str[i]);
				sts = scrollImage (fname, 10);
				if (sts != 0)
					break;
			}
		} while (sts == 0);
		gdispClear (Black);
		scrollCount (0);
		if (sts != 0)
			i2sPlay ("click.snd");
		orchardAppExit ();
	}

	return;
}

static void
ledsign_exit(OrchardAppContext *context)
{
	chHeapFree (context->instance->uicontext->itemlist);
	chHeapFree (context->instance->uicontext);
 	chHeapFree (context->priv);
 
	return;
}

orchard_app("LED Sign", "sign.rgb", 0, ledsign_init, ledsign_start,
    ledsign_event, ledsign_exit, 9999);
