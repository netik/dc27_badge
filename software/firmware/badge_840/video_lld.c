/*-
 * Copyright (c) 2017-2018
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

#include "gfx.h"
#include "src/gdisp/gdisp_driver.h"

#include "ff.h"
#include "ffconf.h"
#include "diskio.h"

#include "video_lld.h"
#include "async_io_lld.h"

int
videoPlay (char * fname)
{
	int i;
	int j;
	FIL f;
	pixel_t * buf;
	pixel_t * p1;
	pixel_t * p2;
	pixel_t * p;
	pixel_t * linebuf;
	UINT br;
	GListener gl;
	GSourceHandle gs;
	GEventMouse * me = NULL;

	buf = chHeapAlloc (NULL, VID_CHUNK_BYTES * 2);
	linebuf = chHeapAlloc (NULL, 640 * 2);

	if (buf == NULL)
 		return (-1);

	if (f_open(&f, fname, FA_READ) != FR_OK) {
		chHeapFree (buf);
		return (-1);
	}

	GDISP->p.x = 0;
	GDISP->p.y = 0;
	GDISP->p.cx = gdispGetWidth ();
	GDISP->p.cy = gdispGetHeight ();

	gdisp_lld_write_start (GDISP);

	gs = ginputGetMouse (0);
	geventListenerInit (&gl);
	geventAttachSource (&gl, gs, GLISTEN_MOUSEMETA);

	p1 = buf;
	p2 = buf + VID_CHUNK_PIXELS;

	/* Pre-load initial chunk */

	f_read(&f, p1, VID_CHUNK_BYTES, &br);

	while (1) {

		if (br == 0)
			break;

		/*
		 * Note: this resets the timer to 0 and starts it
		 * counting.
		 */

		gptStartContinuous (&GPTD2, VID_TIMER_RESOLUTION);

		/* Start next async read */

		asyncIoRead (&f, p2, VID_CHUNK_BYTES, &br);

		/* Draw the current batch of lines to the screen */

                for (j = 0; j < VID_CHUNK_LINES; j++) {
                        p = linebuf;
                        for (i = 0; i < 160; i++) {
                                *p = p1[i + (160 * j)];
                                *(p + 320) = p1[i + (160 * j)];
                                p++;
                                *p = p1[i + (160 * j)];
                                *(p + 320) = p1[i + (160 * j)];
                                p++;
                        }
                        spiSend (&SPID4, 320*4, linebuf);
                }

		/* Switch to next waiting chunk */

		if (p1 == buf) {
			p1 += VID_CHUNK_PIXELS;
			p2 = buf;
		} else {
			p1 = buf;
			p2 += VID_CHUNK_PIXELS;
		}

		/* Wait for async read to complete */

		asyncIoWait ();

		me = (GEventMouse *)geventEventWait (&gl, 0);
		if (me != NULL && me->buttons & GMETA_MOUSE_DOWN)
			break;

		/* Wait for sync timer to expire. */

		while (gptGetCounterX (&GPTD2) < VID_TIMER_INTERVAL)
			chThdSleep (1);
		gptStopTimer (&GPTD2);
	}

	gdisp_lld_write_stop (GDISP);

	geventDetachSource (&gl, NULL);

	f_close (&f);

	/* Stop the timer */

	gptStopTimer (&GPTD2);

	/* Release memory */

	chHeapFree (buf);
	chHeapFree (linebuf);

	if (me != NULL && me->buttons & GMETA_MOUSE_DOWN)
		return (-1);

	return (0);
}
