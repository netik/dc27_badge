/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: svga_disp.c,v 1.3 1997/11/22 14:27:47 ahornby Exp $
******************************************************************************/
#include "ch.h"
#include "hal.h"
#include "badge.h"

#include "gfx.h"
#include "src/gdisp/gdisp_driver.h"

#include "config.h"
#include "version.h"

/* Standard Includes */
/*#include <stdio.h>*/
#include <stdlib.h>
#include <string.h>

/* My headers */
#include "types.h"
#include "vmachine.h"
#include "address.h"
#include "files.h"
#include "colours.h"
#include "keyboard.h"
#include "limiter.h"
#include "options.h"


#define NUMCOLS 256
uint16_t colors[NUMCOLS];

/* Various variables and short functions */

/* Start of image data */
BYTE *vscreen;

/* The width and height of the image, including any magnification */
int vwidth, vheight, theight;

/* The frame rate counter. Used by the -rr switch */
int tv_counter = 0;
int tv_depth = 8;

int tv_bytes_pp = 2;

/* Create the color map of Atari colors */
/* VGA colors are only 6 bits wide */
static void
create_cmap (void)
{
	int i;
	uint8_t r, g, b;
	uint16_t c;

	/*
	 * The ILI9341 uses an RGB565 pixel format. The colortable[]
	 * array contains a list of all possible Atari 2600 color
	 * values in 24-bit RGB format. We convert them here into
	 * the closest RGB565 equivalents.
	 */

	for (i = 0; i < NUMCOLS; i++) {

		r = (colortable[i] & 0xff0000) >> 16;
		g = (colortable[i] & 0x00ff00) >> 8;
		b = (colortable[i] & 0x0000ff);

		r /= 8;
		g /= 4;
		b /= 8;

		c = b;
		c |= g << 5;
		c |= r << 11;

		colors[i] = c;
	}

	return;
}

/* Create the main window */
void
create_window (void)
{
	/* Calculate the video width and height */

	vwidth = base_opts.magstep * tv_width /** 2*/;
	theight = (tv_height /*+ tv_overscan*/);
	vheight = base_opts.magstep * (theight);

	/* Turn on the keyboard. Must be done after toplevel is created */
	init_keyboard ();

	create_cmap ();

	return;
}

/* Turns on the television. */
/* argc: argument count */
/* argv: argument text */
/* returns: 1 for success, 0 for failure */
int
tv_on (int argc, char **argv)
{
	GDISP->p.x = 0;
	GDISP->p.y = 24;

	GDISP->p.cx = vwidth * 2;
	GDISP->p.cy = vheight + 1;

        gdisp_lld_write_start (GDISP);

	return (1);
}


/* Turn off the tv. Closes the X connection and frees the shared memory */
void
tv_off (void)
{
        gdisp_lld_write_stop (GDISP);
	return;
}


/* Translates a 2600 color value into an X11 pixel value */
/* b: byte containing 2600 colour value */
/* returns: X11 pixel value */

unsigned int 
tv_color (BYTE b)
{
	return (b);
}

/* Displays the tv screen */
void
tv_display (void)
{
	tv_counter++;
}

/* The Event code. */
void
tv_event (void)
{
	read_keyboard ();
	return;
}

void
tv_drawline (int line)
{
	uint16_t * p;
	int i;

	if (line > vheight)
		return;

	p = (uint16_t *)vscreen;
	
	for (i = 0; i < vwidth; i++) {
		GDISP->p.color = colors[p[i]];
		gdisp_lld_write_color (GDISP);
		gdisp_lld_write_color (GDISP);
	}

	return;
}

void
tv_drawpixel (pixel_t pixel)
{
	GDISP->p.color = colors[pixel];
	gdisp_lld_write_color (GDISP);
	gdisp_lld_write_color (GDISP);
	return;
}
