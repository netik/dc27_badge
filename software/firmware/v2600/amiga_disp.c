/*****************************************************************************
Author     : Matthew Stroup
Description: Amiga graphics output for v2600 Atari 2600 emulator
Version    : 1.0 Jan 22, 1997
******************************************************************************/

#include "config.h"
#include "version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "vmachine.h"
#include "address.h"
#include "files.h"
#include "colours.h"
#include "keyboard.h"
#include "options.h"
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <graphics/gfxmacros.h>
#include <graphics/display.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

/*****************************************************************************/

void __asm chunky2planar(register __a0 UBYTE *chunky, register __a1 PLANEPTR raster);

struct Screen *scr;
struct Window *win;
struct RastPort *rp,*cdrp;
struct ViewPort *vp;
struct IntuiMessage *message;
struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;

struct TagItem TagArray[] = {
	// can add other tags here
	TAG_DONE,0
};

char perm[8] = {0, 1, 2, 3, 4, 5, 6, 7};	// bitplane order

PLANEPTR raster;			// 8 contiguous bitplanes
struct BitMap bitmap_bm;	// The full depth-8 bitmap

struct ExtNewScreen NewScreenStructure = {
	0,0,
	320,200,
	7,				// depth
	0,1,
	INTERLACE,
	CUSTOMSCREEN+CUSTOMBITMAP+SCREENQUIET+NS_EXTENDED,
	NULL,
	NULL,
	NULL,
	&bitmap_bm,
	(struct TagItem *)&TagArray
};

#define NUMCOLS 128
UWORD colors[NUMCOLS];

/* Start of image data */
UBYTE *vscreen;

/* The width and height of the image, including any magnification */
int vwidth, vheight, theight;

/* The frame rate counter. Used by the -rr switch */
int tv_counter = 0;

/* Optionally set by the X debugger. */
int redraw_flag = 1;

static int bytesize;

long get_screen(void);

extern struct Custom far custom;

/*****************************************************************************/

/* Turns on the television. */
/* returns: 1 for success, 0 for failure */
int tv_on (int argc, char **argv)
{
int i;

	// Make sure necessary ROM libraries are open
	IntuitionBase=(struct IntuitionBase *) OpenLibrary("intuition.library",0);
	if (IntuitionBase == NULL) return(0);	

	GfxBase=(struct GfxBase *) OpenLibrary("graphics.library",0);
	if (GfxBase == NULL) return(0);

	if (get_screen()==0) return(0);

	bytesize = 64000;

	/* Calculate the video width and height */
	vwidth = tv_width * 2;
	theight = tv_height + 8;
	vheight = theight;

	/* Turn on the keyboard. Must be done after toplevel is created */
	init_keyboard();

	for (i=0; i<NUMCOLS; i++)
	{
		colors[i]=((colortable[i*2] & 0xf00000) >> 12) +
				((colortable[i*2] & 0x00f000) >> 8) +
				((colortable[i*2] & 0x0000f0) >> 4);
	}
	LoadRGB4(&scr->ViewPort, colors, NUMCOLS);

	if (Verbose) printf ("OK\n  Allocating screen buffer...");

	vscreen = (UBYTE *) AllocMem (64000, MEMF_PUBLIC|MEMF_CLEAR);

	if (!vscreen)
	{
		if (Verbose) printf ("Memory Allocation FAILED\n");
		return (0);
	}

	if (Verbose) printf ("OK\n");
	return (1);
}

/*****************************************************************************/

/* Turn off the tv. Closes the X connection and frees the shared memory */
void tv_off (void)
{
	if (Verbose) printf ("Switching off...\n");
	if (scr) CloseScreen(scr);
	CloseLibrary((struct Library *)GfxBase);
	CloseLibrary((struct Library *)IntuitionBase);
	if (vscreen) FreeMem(vscreen, 64000);
	if (raster) FreeRaster (raster, 320, 8 * 200);
}

/*****************************************************************************/

/* Displays the tv screen */
void tv_display (void)
{
	/* Only display if the frame is a valid one. */
	if (tv_counter % base_opts.rr == 0)
	{
		if (Verbose) printf ("displaying...\n");
		chunky2planar(vscreen,raster);
	}
	tv_counter++;
}

/*****************************************************************************/

/* The Event code. */
void tv_event (void)
{
	read_keyboard();
}

/*****************************************************************************/

UBYTE tv_color (UBYTE b)
{
	return (UBYTE)(b >> 1);
}

/*****************************************************************************/

long get_screen(void)
{
	long depth, ok = 0;

	InitBitMap(&bitmap_bm, 8, 320, 200);	// Full depth-8 bm

	// since c2p_020() needs 8 contiguous bitplanes, it is not
	// possible to just use OpenScreen() or AllocBitmap() as they
	// may give the bitplanes in a few chunks of memory (noncontiguous)

	if( raster = (PLANEPTR)AllocRaster (320, 8 * 200))
	{
		for(depth = 0; depth < 8; depth++)
		    bitmap_bm.Planes[depth] = raster + (perm[depth] * RASSIZE (320, 200));

		
		if(scr = OpenScreen( (struct NewScreen *) &NewScreenStructure))
		{
			SetRast(&scr->RastPort, 0);		// clear screen memory
			WaitBlit();					// wait until it's finished
	
			ok = 1;
		}
	}

	return(ok);

}
