/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: dos_disp.c,v 1.3 1997/04/06 02:18:05 ahornby Exp $
******************************************************************************/
#include "config.h"
#include "version.h"
/* 
   The DOS Allegro display code. 
 */

/* Standard Includes */
#include <stdio.h>
#include <stdlib.h>
#include <allegro.h>

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
PALLETE colors;
PALLETE oldcol;

/* Various variables and short functions */

/* Start of image data */
BYTE *vscreen;
BITMAP *bmp;

/* The width and height of the image, including any magnification */
int vwidth, vheight, theight;

/* The frame rate counter. Used by the -rr switch */
int tv_counter = 0;

/* Optionally set by the X debugger. */
int redraw_flag = 1;

static int bytesize;

/* Inline helper to place the tv image */
static inline void
put_image (void)
{
  blit(bmp,screen,0,0,0,0,320,200);
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
      colors[i].r = (colortable[i * 2] & 0xff0000) >> 18;
      colors[i].g = (colortable[i * 2] & 0x00ff00) >> 10;
      colors[i].b = (colortable[i * 2] & 0x0000ff) >> 2;
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
  get_palette_range(oldcol,0,NUMCOLS);

  /* Create the color map */
  create_cmap ();

  /* Use the color map */
  set_palette_range(colors,0,NUMCOLS,0);
}


/* Turns on the television. */
/* argc: argument count */
/* argv: argument text */
/* returns: 1 for success, 0 for failure */
int
tv_on (int argc, char **argv)
{

  /* Set up screen mode */
  allegro_init ();
  set_gfx_mode(GFX_VGA,320,200,0,0);

  bytesize = tv_height * 320;
  if (bytesize > 64000)
    bytesize = 64000;

  /* Create the windows */
  create_window (argc, argv);

  bmp=create_bitmap(vwidth,vheight);

  if (Verbose)
    printf ("OK\n  Allocating screen buffer...");

  vscreen = (BYTE *) (bmp->dat);

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
  set_palette_range(oldcol,0,NUMCOLS,0);

  /* Go Text */
  set_gfx_mode(GFX_TEXT,80,25,0,0);
  allegro_exit();
}


/* Translates a 2600 color value into an X11 pixel value */
/* b: byte containing 2600 colour value */
/* returns: X11 pixel value */
BYTE
tv_color (BYTE b)
{
  return (b >> 1);
}

/* Displays the tv screen */
void
tv_display (void)
{
  /* Only display if the frame is a valid one. */
  if (tv_counter % base_opts.rr == 0)
    {
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
