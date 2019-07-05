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
#include "orchard-ui.h"

#include "scroll_lld.h"
#include "nrf52i2s_lld.h"

#include "led.h"
#include "userconfig.h"

static uint32_t
credits_init(OrchardAppContext *context)
{
	(void)context;

	return (0);
}

static void
credits_start(OrchardAppContext *context)
{
	int r;

	(void)context;

        // curtains down, start the show.
	gdispClear (Black);

	ledSetPattern(16); // blue spin
        
	scrollAreaSet (0, 0);

	i2sWait ();
	i2sPlay ("sound/fflost.snd");
	r = scrollImage ("images/credits.rgb", 25);

	chThdSleepMilliseconds (800);

	gdispClear (Black);
	scrollCount (0);

	i2sPlay (NULL);
	if (r == 0)
		i2sPlay (NULL);
	else
		i2sPlay ("sound/click.snd");

	orchardAppExit ();

	return;
}

static void
credits_event(OrchardAppContext *context, const OrchardAppEvent *event)
{
	(void)context;
	(void)event;

	return;
}

static void
credits_exit(OrchardAppContext *context)
{
	(void)context;
	userconfig *config = getConfig();

        // restore leds
	ledSetPattern(config->led_pattern);
	return;
}

orchard_app("Credits", "icons/captain.rgb", 0, credits_init, credits_start,
    credits_event, credits_exit, 3);
