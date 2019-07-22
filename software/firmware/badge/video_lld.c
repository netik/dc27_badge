/*-
 * Copyright (c) 2017-2019
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
#include "nrf52i2s_lld.h"

#include "badge.h"

#include <stdlib.h>

typedef struct _audio_state {
	uint16_t *	audio_curbuf;
	uint16_t *	audio_curframe;
	uint8_t		audio_scnt;
} AUDIO_STATE;

static void
audioAccumulate (AUDIO_STATE * h, uint16_t * samples)
{
	int i;

	for (i = 0; i < VID_AUDIO_SAMPLES_PER_CHUNK; i++)
		h->audio_curframe[i] = samples[i];

	h->audio_curframe += VID_AUDIO_SAMPLES_PER_CHUNK;
	h->audio_scnt++;

	/* If we have enough samples, then start them playing */

	if (h->audio_scnt == VID_AUDIO_BUFCNT) {
		i2sSamplesWait ();
		i2sSamplesPlay (h->audio_curbuf, VID_AUDIO_SAMPLES_PER_CHUNK *
		    VID_AUDIO_BUFCNT);

		/*
		 * Reset the buffers so that we can accumulate
		 * new samples while the current samples play.
		 */

		if (h->audio_curbuf == i2sBuf)
			h->audio_curbuf = i2sBuf +
			    VID_AUDIO_SAMPLES_PER_CHUNK *
			    VID_AUDIO_BUFCNT;
		else
			h->audio_curbuf = i2sBuf;

		h->audio_curframe = h->audio_curbuf;
		h->audio_scnt = 0;
	}

	return;
}

static void
videoAccumulate (pixel_t * pixels, pixel_t * linebuf)
{
	pixel_t pix;
	pixel_t * p;
	int i, j;

	osalSysLock ();
	if (SPID4.state != SPI_READY)
		(void) osalThreadSuspendS (&SPID4.thread);
	osalSysUnlock ();

	if (linebuf == NULL) {
		spiStartSend (&SPID4, VID_CHUNK_LINES *
		    VID_PIXELS_PER_LINE * 2, pixels);
		return;
	}

	for (i = 0; i < VID_CHUNK_LINES; i++) {
		p = linebuf + (320 * i * 2);

		/*
		 * Expand one 160 pixel line to
		 * two 320 pixel lines
		 */

		for (j = 0; j < 160; j++) {
			pix = pixels[j + (160 * i)];;
			p[0] = p[1] = p[320] = p[321] = pix;
			p += 2;
		}
	}

	spiStartSend (&SPID4, 320 * 2 * VID_CHUNK_LINES * 2, linebuf);

	return;
}

int
videoWinPlay (char * fname, int x, int y)
{
	FIL f;
	pixel_t * buf;
	pixel_t * p1;
	pixel_t * p2;
	pixel_t * pcur;
	pixel_t * p;
	pixel_t * linebuf;
	int i;
	UINT br;
	GListener gl;
	GSourceHandle gs;
	GEventMouse * me = NULL;
	AUDIO_STATE * a;

	/* If someone's playing audio, cut them off */

	i2sPlay (NULL);

	/* Allocate memory */

	buf = malloc (VID_CHUNK_BYTES * VID_CACHE_FACTOR * 2);

	if (buf == NULL)
 		return (-1);

	linebuf = malloc (320 * 2 * VID_CHUNK_LINES * 2);

	if (linebuf == NULL) {
		free (buf);
 		return (-1);
	}

	i2sBuf = malloc (VID_AUDIO_BYTES_PER_CHUNK * VID_AUDIO_BUFCNT * 2);

	if (i2sBuf == NULL) {
		free (linebuf);
		free (buf);
 		return (-1);
	}

	if (f_open(&f, fname, FA_READ) != FR_OK) {
		free (buf);
		free (linebuf);
		free (i2sBuf);
		return (-1);
	}

	/* Set the display window */

	if (x == -1) {
		GDISP->p.x = 0;
		GDISP->p.y = 0;
		GDISP->p.cx = gdispGetWidth ();
		GDISP->p.cy = gdispGetHeight ();
	} else {
		GDISP->p.x = x;
		GDISP->p.y = y;
		GDISP->p.cx = VID_PIXELS_PER_LINE;
		GDISP->p.cy = VID_LINES_PER_FRAME;
	}

	gdisp_lld_write_start (GDISP);

	/* Capture mouse/touch events */

	gs = ginputGetMouse (0);
	geventListenerInit (&gl);
	geventAttachSource (&gl, gs, GLISTEN_MOUSEMETA);

	a = malloc (sizeof (AUDIO_STATE));
	a->audio_curbuf = i2sBuf;
	a->audio_curframe = i2sBuf;
	a->audio_scnt = 0;

	p1 = buf;
	p2 = buf + (VID_CHUNK_PIXELS * VID_CACHE_FACTOR);

	/* Pre-load initial chunk */

	f_read(&f, p1, VID_CHUNK_BYTES * VID_CACHE_FACTOR, &br);

	/* Power up the audio amp */

	i2sAudioAmpCtl (I2S_AMP_ON);

	badge_sleep_disable ();

	while (1) {

		if (br == 0)
			break;

		/* Start next async read */

		asyncIoRead (&f, p2, VID_CHUNK_BYTES * VID_CACHE_FACTOR, &br);

		/* Accumulate audio sample data */

		pcur = p1;

		for (i = 0; i < VID_CACHE_FACTOR; i++) {

			/* Accumulate/play audio samples */

			p = pcur + VID_PIXELS_PER_CHUNK;

			audioAccumulate (a, p);

			/* Draw the current batch of lines to the screen */

			if (x == -1)
				videoAccumulate (pcur, linebuf);
			else
				videoAccumulate (pcur, NULL);

			pcur += VID_CHUNK_PIXELS;
		}

		/* Switch to next waiting chunk */

		if (p1 == buf) {
			p1 += (VID_CHUNK_PIXELS * VID_CACHE_FACTOR);
			p2 = buf;
		} else {
			p1 = buf;
			p2 += (VID_CHUNK_PIXELS * VID_CACHE_FACTOR);
		}

		/* Wait for async read to complete */

		asyncIoWait ();

		me = (GEventMouse *)geventEventWait (&gl, 0);
		if (me != NULL && me->buttons & GMETA_MOUSE_DOWN)
			break;
	}

	badge_sleep_enable ();

	/* Drain any pending I/O */

	i2sSamplesWait ();
	i2sSamplesStop ();
	osalSysLock ();
	if (SPID4.state != SPI_READY)
		(void) osalThreadSuspendS (&SPID4.thread);
	osalSysUnlock ();

	/* Power down the audio amp */

	i2sAudioAmpCtl (I2S_AMP_OFF);

	gdisp_lld_write_stop (GDISP);

	geventDetachSource (&gl, NULL);

	f_close (&f);

	/* Release memory */

	free (buf);
	free (linebuf);
	free (i2sBuf);
	free (a);
	i2sBuf = NULL;

	if (me != NULL && me->buttons & GMETA_MOUSE_DOWN)
		return (-1);

	return (0);
}

int
videoPlay (char * fname)
{
	return (videoWinPlay (fname, -1, -1));
}
