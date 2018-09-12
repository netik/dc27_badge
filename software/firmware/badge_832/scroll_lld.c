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

#include "gfx.h"
#include "board_ILI9341.h"

#include "ff.h"
#include "ffconf.h"

#include "scroll_lld.h"

#define ILI9341_VSDEF	0x33
#define ILI9341_VSADD	0x37

static int scroll_pos;

typedef struct gdisp_image {
        uint8_t                 gdi_id1;
        uint8_t                 gdi_id2;
        uint8_t                 gdi_width_hi;
        uint8_t                 gdi_width_lo;
        uint8_t                 gdi_height_hi;
        uint8_t                 gdi_height_lo;
        uint8_t                 gdi_fmt_hi;
        uint8_t                 gdi_fmt_lo;
} GDISP_IMAGE;

void
scrollAreaSet (uint16_t TFA, uint16_t BFA)
{
	init_board (NULL);

	acquire_bus (NULL);
	write_index (NULL, ILI9341_VSDEF);
	write_data16 (NULL, TFA);
	write_data16 (NULL, 320 - TFA - BFA);
	write_data16 (NULL, BFA);
	release_bus (NULL);

	scroll_pos = gdispGetWidth() - 1;

	return;
}

void
scrollCount (int lines)
{
	acquire_bus (NULL);
	write_index (NULL, ILI9341_VSADD);
	write_data (NULL, lines >> 8);
	write_data (NULL, (uint8_t)lines);
	release_bus (NULL);

	return;
}

int
scrollImage (char * file, int delay)
{
	int i;
	FIL f;
 	UINT br;
	GDISP_IMAGE * hdr;
	uint16_t h;
	uint16_t w;
	pixel_t * buf;
	orientation_t o;
	GEventMouse * me;
	GSourceHandle gs;
	GListener gl;

	if (f_open (&f, file, FA_READ) != FR_OK)
		return (-1);

	gs = ginputGetMouse (0);
	geventListenerInit (&gl);
	geventAttachSource (&gl, gs, GLISTEN_MOUSEMETA);

	buf = chHeapAlloc (NULL, sizeof(pixel_t) * gdispGetHeight ());

	hdr = (GDISP_IMAGE *)buf;
	f_read (&f, hdr, sizeof(GDISP_IMAGE), &br);
	h = hdr->gdi_height_hi << 8 | hdr->gdi_height_lo;
	w = hdr->gdi_width_hi << 8 | hdr->gdi_width_lo;

	o = gdispGetOrientation();
	me = NULL;

	/* Sanity check for bogus images */

	if (hdr->gdi_id1 != 'N' || hdr->gdi_id2 != 'I' || w > 240)
		goto out;

	for (i = 0; i < h; i++) {
		f_read (&f, buf, sizeof(pixel_t) * w, &br);
		gdispBlitAreaEx (scroll_pos, (gdispGetHeight () - w) / 2, 1,
		    w, 0, 0, 1, buf);
		scroll_pos++;
		if (scroll_pos == gdispGetWidth ())
			scroll_pos = 0;
		scrollCount (o == GDISP_ROTATE_90 ?
			(gdispGetWidth() - 1) - scroll_pos : scroll_pos);
		me = (GEventMouse *)geventEventWait(&gl, 0);
		if (me != NULL &&  me->buttons & GMETA_MOUSE_DOWN)
			break;
		chThdSleepMilliseconds (delay);
	}

	geventDetachSource (&gl, NULL);

out:

	chHeapFree (buf);
	f_close (&f);

	if (me != NULL &&  me->buttons & GMETA_MOUSE_DOWN)
		return (-1);

	return (0);
}
