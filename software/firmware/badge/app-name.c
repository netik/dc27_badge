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

#include <stdlib.h>

#include "orchard-app.h"
#include "orchard-ui.h"
#include "userconfig.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"

static uint32_t name_init(OrchardAppContext *context)
{
	(void)context;

	/*
	 * We don't want any extra stack space allocated for us.
	 * We'll use the heap.
	 */

	return (0);
}

static void name_start(OrchardAppContext *context)
{
	OrchardUiContext * keyboardUiContext;
	userconfig * config;

	config = getConfig();

	keyboardUiContext = malloc (sizeof(OrchardUiContext));
	keyboardUiContext->itemlist = (const char **)malloc (
            sizeof(char *) * 2);
	keyboardUiContext->itemlist[0] =
	    "Ahoy! Your name, Captain?\nTap ENTER when done.";
	keyboardUiContext->itemlist[1] = config->name;
	keyboardUiContext->total = CONFIG_NAME_MAXLEN;

	context->instance->ui = getUiByName("keyboard");
	context->instance->uicontext = keyboardUiContext;
	context->instance->ui->start (context);

	return;
}

static void name_event(OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	const OrchardUi * keyboardUi;
	userconfig * config;
	  
	keyboardUi = context->instance->ui;
	config = getConfig();

	if (event->type == ugfxEvent)
		keyboardUi->event (context, event);

	if (event->type == appEvent && event->app.event == appTerminate)
		return;

	if (event->type == uiEvent) {
		if ((event->ui.code == uiComplete) &&
		    (event->ui.flags == uiOK)) {
			configSave (config);
			bleGapUpdateName ();
                        if (config->p_type == p_notset)
				orchardAppRun (orchardAppByName("Choosetype"));
			else
				orchardAppExit ();
		}
	}

	return;
}

static void name_exit(OrchardAppContext *context)
{
	const OrchardUi * keyboardUi;
  
	keyboardUi = context->instance->ui;
	keyboardUi->exit (context);

	free (context->instance->uicontext->itemlist);
	free (context->instance->uicontext);
  
	return;
}

orchard_app("Set your name", NULL, APP_FLAG_HIDDEN, name_init,
    name_start, name_event, name_exit, 9999);
