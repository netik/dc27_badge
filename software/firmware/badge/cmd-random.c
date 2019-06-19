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

#include "nrf_soc.h"
#include "nrf_sdm.h"

#include "badge.h"
#include "math.h"
#include "rand.h"

/*
 * Command Random
 */

#define RANDOM_BUFFER_SIZE 16
static uint8_t random_buffer[RANDOM_BUFFER_SIZE];

static void
cmd_random (BaseSequentialStream *chp, int argc, char *argv[])
{
	uint8_t size = 16;
	uint16_t i    = 0;
	uint8_t  nl   = 0;
	uint8_t	sd_size;
 	uint8_t sdenabled;

	if (argc > 0)
		size = atoi(argv[0]);

	rngAcquireUnit (&RNGD1);

	sd_softdevice_is_enabled (&sdenabled);

	if (sdenabled) {
		sd_rand_application_bytes_available_get (&sd_size);

		if (sd_size > RANDOM_BUFFER_SIZE)
			sd_size = RANDOM_BUFFER_SIZE;

		if (size > sd_size) {
			printf ("random: maximum size is %d.\r\n", sd_size);
			return;
		}

		printf ("Fetching %d random byte(s):\r\n", size);

		sd_rand_application_vector_get (random_buffer, size);
	} else {
		if (size > RANDOM_BUFFER_SIZE) {
			printf ("random: maximum size is %d.\r\n",
			    RANDOM_BUFFER_SIZE);
			return;
		}

		printf ("Fetching %d random byte(s):\r\n", size);
		rngWrite (&RNGD1, random_buffer, size, OSAL_MS2I(100));
	}

	rngReleaseUnit (&RNGD1);

	for (i = 0 ; i < size ; i++) {
		printf ("%02x ", random_buffer[i]);
		if ((nl = (((i+1) % 20)) == 0))
			printf ("\r\n");
	}
	if (!nl)
		printf ("\r\n");

  	printf("range-random: %d \r\n", randRange(0,100));
  	printf("rand(): %d\r\n", randByte());

	return;
}

orchard_command ("random", cmd_random);
