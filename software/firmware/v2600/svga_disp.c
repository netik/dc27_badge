/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: svga_disp.c,v 1.3 1997/11/22 14:27:47 ahornby Exp $
******************************************************************************/
#include "config.h"
#include "version.h"
/* 
   The SVGALIB display code. 
 */

/* Standard Includes */
#include <stdio.h>
#include <stdlib.h>
#include <vga.h>

/* My headers */
#include "types.h"
#include "vmachine.h"
#include "address.h"
#include "files.h"
#include "colours.h"
#include "keyboard.h"
#include "limiter.h"
#include "options.h"


struct AlColor
  {
    int red, green, blue;
  };

#define NUMCOLS 128
struct AlColor colors[NUMCOLS];
struct AlColor oldcol[NUMCOLS];

/* Various variables and short functions */

/* Start of image data */
BYTE *vscreen;

/* The width and height of the image, including any magnification */
int vwidth, vheight, theight;

/* The frame rate counter. Used by the -rr switch */
int tv_counter = 0;
int tv_depth = 8;

/* Optionally set by the X debugger. */
int redraw_flag = 1;

int tv_bytes_pp=1;

static int bytesize;

/* Inline helper to place the tv image */
static inline void
put_image (void)
{
  memcpy (graph_mem, vscreen, bytesize);
}

/* Create the color map of Atari colors */
/* VGA colors are only 6 bits wide */
static void
create_cmap (void)
{
  int i;

  /* Initialise parts of the colors array */
  for (i = 0; i < NUMCOLS; i++)
    {
      /* Use the color values from the color table */
      colors[i].red = (colortable[i * 2] & 0xff0000) >> 18;
      colors[i].green = (colortable[i * 2] & 0x00ff00) >> 10;
      colors[i].blue = (colortable[i * 2] & 0x0000ff) >> 2;
    }
}

/* Create the main window */
/* argc: argument count */
/* argv: argument text */
static void
create_window (int argc, char **argv)
{
  int i;

  /* Calculate the video width and height */
  vwidth = base_opts.magstep * tv_width * 2;
  theight = tv_height + 8;
  vheight = base_opts.magstep * theight;

  /* Turn on the keyboard. Must be done after toplevel is created */
  init_keyboard ();

  /* Save the current color map */
  for (i = 0; i < NUMCOLS; i++)
    vga_getpalette (i, &oldcol[i].red, &oldcol[i].green, &oldcol[i].blue);

  /* Create the color map */
  create_cmap ();

  /* Use the color map */
  for (i = 0; i < NUMCOLS; i++)
    vga_setpalette (i, colors[i].red, colors[i].green, colors[i].blue);
}


/* Turns on the television. */
/* argc: argument count */
/* argv: argument text */
/* returns: 1 for success, 0 for failure */
int
tv_on (int argc, char **argv)
{

  /* Set up screen mode */
  vga_init ();
  vga_setmode (G320x200x256);
  vga_clear ();

  bytesize = tv_height * 320;
  if (bytesize > 64000)
    bytesize = 64000;

  /* Create the windows */
  create_window (argc, argv);

  if (Verbose)
    printf ("OK\n  Allocating screen buffer...");

  vscreen = (BYTE *) malloc (sizeof (BYTE) * vheight * vwidth);

  if (!vscreen)
    {
      if (Verbose)
	printf ("Memory Allocation FAILED\n");
      return (0);
    }

  if (Verbose)
    printf ("OK\n");
  return (1);
}


/* Turn off the tv. Closes the X connection and frees the shared memory */
void
tv_off (void)
{
  int i;
  if (Verbose)
    printf ("Switching off...\n");

  /* Restore the color map */
  for (i = 0; i < NUMCOLS; i++)
    vga_setpalette (i, oldcol[i].red, oldcol[i].green, oldcol[i].blue);

  /* Go Text */
  vga_setmode (TEXT);
}


/* Translates a 2600 color value into an X11 pixel value */
/* b: byte containing 2600 colour value */
/* returns: X11 pixel value */
unsigned int 
tv_color (BYTE b)
{
  return ((b>>1)<<8)+(b>>1);
}

/* Displays the tv screen */
void
tv_display (void)
{
  /* Only display if the frame is a valid one. */
  if (tv_counter % base_opts.rr == 0)
    {
      if (Verbose)
	printf ("displaying...\n");
      put_image ();
    }
  tv_counter++;
}

/* The Event code. */
void
tv_event (void)
{
  read_keyboard ();
}


/* Single pixel plotting function. Used for debugging,  */
/* not in production code  */
/* x: horizontal position */
/* y: vertical position */
/* value: pixel value */
void
tv_putpixel (int x, int y, BYTE value)
{
  BYTE *p;

  switch (base_opts.magstep)
    {
    case 1:
      x = x << 1;
      p = vscreen + x + y * vwidth;
      *(p) = value;
      *(p + 1) = value;
    }
}
