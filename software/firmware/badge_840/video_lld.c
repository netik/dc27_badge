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
#include "i2s_lld.h"

static int audio_start;

static void
audioPlay (void * p, int cnt)
{
	if (audio_start == 0) {
		audio_start++;
		i2sSamplesSend (p, cnt);
	} else {
		/* Wait for audio to finish playing. */
		i2sSamplesWait ();
		i2sSamplesNext (p, cnt);
	}

	return;
}

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
	uint16_t * cur;
	uint16_t * ps;
	int scnt;
	UINT br;
	GListener gl;
	GSourceHandle gs;
	GEventMouse * me = NULL;

	/* If someone's playing audio, cut them off */

	i2sPlay (NULL);

	/* Allocate memory */

	buf = chHeapAlloc (NULL, VID_CHUNK_BYTES * 2);

	if (buf == NULL)
 		return (-1);

	linebuf = chHeapAlloc (NULL, 320 * 2 * VID_CHUNK_LINES * 2);

	if (linebuf == NULL) {
		chHeapFree (buf);
 		return (-1);
	}

	i2sBuf = chHeapAlloc (NULL,
	    VID_AUDIO_BYTES_PER_CHUNK * VID_AUDIO_BUFCNT * 2);

	if (i2sBuf == NULL) {
		chHeapFree (linebuf);
		chHeapFree (buf);
 		return (-1);
	}

	if (f_open(&f, fname, FA_READ) != FR_OK) {
		chHeapFree (buf);
		chHeapFree (linebuf);
		chHeapFree (i2sBuf);
		return (-1);
	}

	/* Set the display window */

	GDISP->p.x = 0;
	GDISP->p.y = 0;
	GDISP->p.cx = gdispGetWidth ();
	GDISP->p.cy = gdispGetHeight ();

	gdisp_lld_write_start (GDISP);

	/* Capture mouse/touch events */

	gs = ginputGetMouse (0);
	geventListenerInit (&gl);
	geventAttachSource (&gl, gs, GLISTEN_MOUSEMETA);

	p1 = buf;
	p2 = buf + VID_CHUNK_PIXELS;
	cur = i2sBuf;
	ps = cur;
	scnt = 0;

	/* Pre-load initial chunk */

	f_read(&f, p1, VID_CHUNK_BYTES, &br);

	audio_start = 0;

	while (1) {

		if (br == 0)
			break;

		/* Start next async read */

		asyncIoRead (&f, p2, VID_CHUNK_BYTES, &br);

		/* Accumulate audio sample data */

		p = p1 + VID_PIXELS_PER_CHUNK;

		for (i = 0; i < VID_AUDIO_SAMPLES_PER_CHUNK; i++)
			ps[i] = p[i];
		ps += VID_AUDIO_SAMPLES_PER_CHUNK;
		scnt++;

		/* If we have enough, then start it playing */

		if (scnt == VID_AUDIO_BUFCNT) {
			audioPlay (cur, VID_AUDIO_SAMPLES_PER_CHUNK *
			    VID_AUDIO_BUFCNT);
			if (cur == i2sBuf)
				cur = i2sBuf +
				    VID_AUDIO_SAMPLES_PER_CHUNK *
				    VID_AUDIO_BUFCNT;
			else
				cur = i2sBuf;
			ps = cur;
			scnt = 0;
		}

		/* Wait for display bus to come ready */

		while (SPID4.state != SPI_READY)
			chThdSleep (1);

		/* Draw the current batch of lines to the screen */

		for (j = 0; j < VID_CHUNK_LINES; j++) {
			pixel_t pix;
			p = linebuf + (320 * j * 2);

			/* Expand one 160 pixel line to two 320 pixel lines */

			for (i = 0; i < 160; i++) {
				pix =  p1[i + (160 * j)];;
				p[0] = p[1] = p[320] = p[321] = pix;
				p += 2;
			}
		}

		spiStartSend (&SPID4, 320 * 2 * VID_CHUNK_LINES * 2, linebuf);

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
	}

	/* Drain any pending I/O */

	i2sSamplesWait ();
	while (SPID4.state != SPI_READY)
		chThdSleep (1);

	gdisp_lld_write_stop (GDISP);

	geventDetachSource (&gl, NULL);

	f_close (&f);

	/* Release memory */

	chHeapFree (buf);
	chHeapFree (linebuf);
	chHeapFree (i2sBuf);
	i2sBuf = NULL;

	if (me != NULL && me->buttons & GMETA_MOUSE_DOWN)
		return (-1);

	return (0);
}
