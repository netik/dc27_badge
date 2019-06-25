/*-
 * Copyright (c) 2017, 2019
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
#include "video_lld.h"

#include "nrf52i2s_lld.h"

#include "ff.h"
#include "ffconf.h"
#include "led.h"
#include "userconfig.h"

#include <string.h>

typedef struct _VideoHandles {
	char **			listitems;
	int			itemcnt;
	OrchardUiContext	uiCtx;
} VideoHandles;

static char * videodir;

static uint32_t
video_civildef(OrchardAppContext *context)
{
	(void)context;
	videodir = "/videos/civildef";
	return (0);
}

static uint32_t
video_dabomb(OrchardAppContext *context)
{
	(void)context;
	videodir = "/videos/dabomb";
	return (0);
}

static uint32_t
video_misc(OrchardAppContext *context)
{
	(void)context;
	videodir = "/videos/misc";
	return (0);
}

static void
video_start (OrchardAppContext *context)
{
	VideoHandles * p;
	DIR d;
	FILINFO info;
	int i;

	f_opendir (&d, videodir);

	i = 0;

	while (1) {
		if (f_readdir (&d, &info) != FR_OK || info.fname[0] == 0)
			break;
		if (strstr (info.fname, ".VID") != NULL)
			i++;
	}

	f_closedir (&d);

	if (i == 0) {
		orchardAppExit ();
		return;
	}

	i += 2;
	ledStop();

	p = malloc (sizeof(VideoHandles));
	memset (p, 0, sizeof(VideoHandles));
	p->listitems = malloc (sizeof(char *) * i);
	p->itemcnt = i;

	p->listitems[0] = "Choose a video";
	p->listitems[1] = "Exit";

	f_opendir (&d, videodir);

	i = 2;

	while (1) {
		if (f_readdir (&d, &info) != FR_OK || info.fname[0] == 0)
			break;
		if (strstr (info.fname, ".VID") != NULL) {
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
video_event(OrchardAppContext *context, const OrchardAppEvent *event)
{
	OrchardUiContext * uiContext;
	const OrchardUi * ui;
	VideoHandles * p;
	char videofn[35];

	p = context->priv;
	ui = context->instance->ui;
	uiContext = context->instance->uicontext;

	if (event->type == appEvent && event->app.event == appTerminate) {
		if (ui != NULL)
 			ui->exit (context);
	}

	if (event->type == ugfxEvent || event->type == keyEvent)
		if (ui != NULL)
			ui->event (context, event);

	if (event->type == uiEvent && event->ui.code == uiComplete &&
	    event->ui.flags == uiOK && ui != NULL) {
		/*
		 * If this is the list ui exiting, it means we chose a
		 * video to play, or else the user selected exit.
		 */
		ui->exit (context);
		context->instance->ui = NULL;

		/* User chose the "EXIT" selection, bail out. */
		if (uiContext->selected == 0) {
			orchardAppExit ();
			return;
		}

		strcpy (videofn, videodir);
		strcat (videofn, "/");
		strcat (videofn, p->listitems[uiContext->selected +1]);

		if (videoPlay (videofn) != 0)
			i2sPlay ("sound/click.snd");

		orchardAppExit ();
	}

	return;
}

static void
video_exit(OrchardAppContext *context)
{
	VideoHandles * p;
	int i;
	userconfig *config = getConfig();

	p = context->priv;

	if (p == NULL)
            return;

	for (i = 2; i < p->itemcnt; i++)
		free (p->listitems[i]);

	free (p->listitems);
	free (p);

	context->priv = NULL;

	ledSetPattern(config->led_pattern);
	return;
}

/*
 * We want to have several categories of videos that the user can
 * select from the launcher, but we don't want to duplicate the video
 * player code. Instead, we add the video player to the Orchard app
 * linker set multiple times, each time with a different name. We
 * can then select what subdirectory to search in the video_init()
 * routine based on instance.app->name.
 *
 * There's a little bit of hackery going on here: the orchard_app()
 * macro builds the linker set symbol name using the names of the
 * init/start/event/exit functions. If these function names are always
 * the same, you'll end up with duplicate symbol names, which will
 * cause a compiler error. To work around this, we use a different
 * name for the init function. This also allows us to use a different
 * subdirectory for each category of videos.
 */

orchard_app("Be Prepared!", "icons/cd.rgb", APP_FLAG_UNLOCK,
    video_civildef, video_start,
    video_event, video_exit, 0);

orchard_app("Da Bomb!", "icons/mask.rgb", 0, video_dabomb, video_start,
    video_event, video_exit, 0);

orchard_app("Misc Videos", "icons/mask.rgb", 0, video_misc, video_start,
    video_event, video_exit, 0);
