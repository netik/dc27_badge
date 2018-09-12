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

#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"

#include "badge.h"

/*
 * Command Watchdog
 */

extern bool watchdog_started;

static void
watchdog_callback(void)
{
	return;
}

WDGConfig WDG_config = {
    .pause_on_sleep = 0,
    .pause_on_halt  = 0,
    .timeout_ms     = 5000,
    .callback       = watchdog_callback,
};

static void
cmd_watchdog(BaseSequentialStream *chp, int argc, char *argv[])
{
    unsigned int timeout = atoi(argv[1]);
	if ((argc != 2) || (strcmp(argv[0], "start"))) {
		usage:
		printf ("Usage: watchdog start <timeout>\r\n"
		    "  <timeout> = 0..%d seconds\r\n",
		    WDG_MAX_TIMEOUT_MS/1000);
		return;
	}

	if (timeout > (WDG_MAX_TIMEOUT_MS/1000))
		goto usage;

	if (watchdog_started) {
		printf ("Watchdog already started."
 		    " Can't be modified once activated.\r\n");
		return;
	}
    
	printf ("Watchdog started\r\n"
		"You need to push BTN1 every %d second(s)\r\n", timeout);

	WDG_config.timeout_ms = timeout * 1000;
	wdgStart(&WDGD1, &WDG_config);
	watchdog_started = true;

	return;
}

orchard_command ("watchdog", cmd_watchdog);
