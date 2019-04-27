/*-
 * Copyright (c) 2018
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

#include <string.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"

#include "i2s_lld.h"

#include "badge.h"

static void
audio_play (int argc, char *argv[])
{
	i2sPlay ("sound/0:DRWHO.SND");
	return;
}

static void
audio_stop (int argc, char *argv[])
{
	i2sPlay (NULL);
	return;
}

static void
cmd_audio(BaseSequentialStream *chp, int argc, char *argv[])
{
	if (argc == 0) {
		printf ("Audio commands:\r\n");
		printf ("play                 Play tune\r\n");
		printf ("stop                 Stop playing\r\n");
                return;
        }

	if (strcmp (argv[0], "play") == 0)
		audio_play (argc, argv);
	else if (strcmp (argv[0], "stop") == 0)
		audio_stop (argc, argv);
	else
		printf ("Unrecognized audio command\r\n");

	return;
}

orchard_command("audio", cmd_audio);
