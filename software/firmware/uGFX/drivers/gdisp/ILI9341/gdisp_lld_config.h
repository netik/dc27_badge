/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_CONFIG_H
#define _GDISP_LLD_CONFIG_H

#if GFX_USE_GDISP

/*===========================================================================*/
/* Driver hardware support.                                                  */
/*===========================================================================*/

#define GDISP_HARDWARE_STREAM_WRITE		TRUE
#define GDISP_HARDWARE_STREAM_READ		TRUE
//#define GDISP_HARDWARE_DRAWPIXEL                TRUE
//#define GDISP_HARDWARE_PIXELREAD                TRUE
#define GDISP_HARDWARE_CONTROL			TRUE
//#define GDISP_HARDWARE_FLUSH			TRUE

#define GDISP_LLD_PIXELFORMAT			GDISP_PIXELFORMAT_RGB565

typedef struct fbInfo {
	void *  pixels;		/* The pixel buffer */
	coord_t linelen;	/* The number of bytes per display line */
} fbInfo;

typedef struct fbPriv {
	fbInfo  fbi;		/* Display information */
} fbPriv;

extern int gdisp_flush_pause;

#endif	/* GFX_USE_GDISP */

#endif	/* _GDISP_LLD_CONFIG_H */
