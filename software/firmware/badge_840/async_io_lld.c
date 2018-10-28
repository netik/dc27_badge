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

#include "ch.h"
#include "hal.h"

#include "ff.h"
#include "ffconf.h"
#include "diskio.h"

#include "async_io_lld.h"

static thread_t * pThread;

static void * async_buf;
static FIL * async_f;
static UINT async_btr;
static volatile UINT async_br = ASYNC_THD_READY;

static UINT * saved_br;

static thread_reference_t fsReference;

static THD_WORKING_AREA(waAsyncIoThread, 256);
static THD_FUNCTION(asyncIoThread, arg)
{
	UINT br;

	(void) arg;

	chRegSetThreadName ("AsyncIo");

	while (1) {
		osalSysLock ();
		async_br = br;
		osalThreadSuspendS (&fsReference);
		osalSysUnlock ();
		if (async_br == ASYNC_THD_EXIT)
			break;
		f_read (async_f, async_buf, async_btr, &br);
	}

	chThdExit (MSG_OK);

        return;
}

void
asyncIoRead (FIL * f, void * buf, UINT btr, UINT * br)
{
	if (async_br != ASYNC_THD_READY)
		return;

	async_f = f;
	async_buf = buf;
	async_btr = btr;

	saved_br = br;

	osalSysLock ();
	async_br = ASYNC_THD_READ;
	osalThreadResumeS (&fsReference, MSG_OK);
	osalSysUnlock ();

	return;
}

void
asyncIoWait (void)
{
	while (async_br == ASYNC_THD_READ)
		;

	*saved_br = async_br;
	async_br = ASYNC_THD_READY;

	return;
}

void
asyncIoStart (void)
{
	pThread = chThdCreateStatic (waAsyncIoThread, sizeof(waAsyncIoThread),
	    NORMALPRIO + 5, asyncIoThread, NULL);

	async_br = ASYNC_THD_READY;

	return;
}

void
asyncIoStop (void)
{
	osalSysLock ();
	async_br = ASYNC_THD_EXIT;
	osalThreadResumeS (&fsReference, MSG_OK);
	osalSysUnlock ();

	chThdWait (pThread);

	pThread = NULL;

	return;
}
