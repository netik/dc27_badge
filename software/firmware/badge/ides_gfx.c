/* Ides of March Badge
 *
 * Shared Graphics Routines
 * J. Adams <1/2017>
 */
#include "ch.h"
#include "hal.h"

#include "orchard-app.h"
#include "string.h"
#include "fontlist.h"
#include "ides_gfx.h"
#include "scroll_lld.h"

#include "gfx.h"
#include "src/gdisp/gdisp_driver.h"

#include "ffconf.h"
#include "ff.h"

#include "async_io_lld.h"
#include "badge.h"

// WidgetStyle: RedButton, the only button we really use
const GWidgetStyle RedButtonStyle = {
  HTML2COLOR(0xff0000),              // background
  HTML2COLOR(0xff6666),              // focus

  // Enabled color set
  {
    HTML2COLOR(0xffffff),         // text
    HTML2COLOR(0x800000),         // edge
    HTML2COLOR(0xff0000),         // fill
    HTML2COLOR(0x008000),         // progress (inactive area)
  },

  // Disabled color set
  {
    HTML2COLOR(0x808080),         // text
    HTML2COLOR(0x404040),         // edge
    HTML2COLOR(0x404040),         // fill
    HTML2COLOR(0x004000),         // progress (active area)
  },

  // Pressed color set
  {
    HTML2COLOR(0xFFFFFF),         // text
    HTML2COLOR(0x800000),         // edge
    HTML2COLOR(0xff6a71),         // fill
    HTML2COLOR(0x008000),         // progress (active area)
  }
};

const GWidgetStyle DarkPurpleStyle = {
  HTML2COLOR(0x470b67),              // background
  HTML2COLOR(0x2ca0ff),              // focus

  // Enabled color set
  {
    HTML2COLOR(0xc8c8c8),         // text
    HTML2COLOR(0x3d2e44),         // edge
    HTML2COLOR(0x808080),         // fill
    HTML2COLOR(0x008000),         // progress (inactive area)
  },

  // Disabled color set
  {
    HTML2COLOR(0x808080),         // text
    HTML2COLOR(0x404040),         // edge
    HTML2COLOR(0x404040),         // fill
    HTML2COLOR(0x004000),         // progress (active area)
  },

  // Pressed color set
  {
    HTML2COLOR(0xFFFFFF),         // text
    HTML2COLOR(0xC0C0C0),         // edge
    HTML2COLOR(0xE0E0E0),         // fill
    HTML2COLOR(0x008000),         // progress (active area)
  }
};

const GWidgetStyle DarkPurpleFilledStyle = {
  HTML2COLOR(0x470b67),              // background
  HTML2COLOR(0x2ca0ff),              // focus

  // Enabled color set
  {
    HTML2COLOR(0xc8c8c8),         // text
    HTML2COLOR(0x3d2e44),         // edge
    HTML2COLOR(0x470b67),         // fill
    HTML2COLOR(0x008000),         // progress (inactive area)
  },

  // Disabled color set
  {
    HTML2COLOR(0x808080),         // text
    HTML2COLOR(0x404040),         // edge
    HTML2COLOR(0x404040),         // fill
    HTML2COLOR(0x004000),         // progress (active area)
  },

  // Pressed color set
  {
    HTML2COLOR(0xFFFFFF),         // text
    HTML2COLOR(0xC0C0C0),         // edge
    HTML2COLOR(0xE0E0E0),         // fill
    HTML2COLOR(0x008000),         // progress (active area)
  }
};

// WidgetStyle: Ivory
const GWidgetStyle IvoryStyle = {
  HTML2COLOR(0xffefbe),              // background
  HTML2COLOR(0x2A8FCD),              // focus

  // Enabled color set
  {
    HTML2COLOR(0x000000),         // text
    HTML2COLOR(0x404040),         // edge
    HTML2COLOR(0xffefbe),         // background
    HTML2COLOR(0x00E000),         // progress (inactive area)
  },

  // Disabled color set
  {
    HTML2COLOR(0xC0C0C0),         // text
    HTML2COLOR(0x808080),         // edge
    HTML2COLOR(0xE0E0E0),         // fill
    HTML2COLOR(0xC0E0C0),         // progress (active area)
  },

  // Pressed color set
  {
    HTML2COLOR(0x404040),         // text
    HTML2COLOR(0x404040),         // edge
    HTML2COLOR(0x808080),         // fill
    HTML2COLOR(0x00E000),         // progress (active area)
  }
};

int
putImageFile (char *name, int16_t x, int16_t y)
{
#ifdef notdef
	gdispImage img;

	if (gdispImageOpenFile (&img, name) == GDISP_IMAGE_ERR_OK) {
		gdispImageDraw (&img, x, y, img.width, img.height, 0, 0);
		gdispImageClose (&img);
		return (1);
	}
#else
	FIL f;
	uint16_t h;
	uint16_t w;
	GDISP_IMAGE hdr;
	uint8_t * buf;
	uint8_t * p1, * p2;
	size_t len;
	UINT br1, br2;

	if (f_open (&f, name, FA_READ) != FR_OK)
		return (1);

	f_read (&f, &hdr, sizeof(GDISP_IMAGE), &br1);
	h = hdr.gdi_height_hi << 8 | hdr.gdi_height_lo;
	w = hdr.gdi_width_hi << 8 | hdr.gdi_width_lo;

	len = 2048 /*sizeof(pixel_t) * w * 8*/;

	buf = malloc (len * 2);

	p1 = buf;
	p2 = buf + len;

	/* Set the display window */

	GDISP->p.x = x;
	GDISP->p.y = y;
 	GDISP->p.cx = w;
 	GDISP->p.cy = h;

	gdisp_lld_write_start (GDISP);
	f_read (&f, buf, len, &br1);

	while (1) {
		asyncIoRead (&f, p2, len, &br2);

		spiSend (&SPID4, br1, p1);

		if (p1 == buf) {
			p1 += len;
			p2 = buf;
		} else {
			p1 = buf;
			p2 += len;
		}

		asyncIoWait ();

		if (br2 == 0)
			break;

		br1 = br2;
	}

	gdisp_lld_write_stop (GDISP);

	f_close (&f);

	free (buf);
#endif
	return (0);
}

void
screen_alert_draw (uint8_t clear, char *msg)
{
	uint16_t middle;
	font_t fontFF;

	middle = (gdispGetHeight() >> 1);
	fontFF = gdispOpenFont (FONT_FIXED);


	if (clear) {
		gdispClear(Black);
	} else {
		/* just black out the drawing area. */
		gdispFillArea( 0, middle - 20, 320, 40, Black );
	}

	gdispDrawThickLine (0, middle - 20, 320, middle -20, Red, 2, FALSE);
	gdispDrawThickLine (0, middle + 20, 320, middle +20, Red, 2, FALSE);

	gdispDrawStringBox (0,
	    middle - (gdispGetFontMetric(fontFF, fontHeight) >> 1),
	    gdispGetWidth(), gdispGetFontMetric(fontFF, fontHeight),
	    msg, fontFF, Red, justifyCenter);


	gdispCloseFont (fontFF);

	return;
}


void drawProgressBar(coord_t x, coord_t y,
                     coord_t width, coord_t height,
                     int32_t maxval, int32_t currentval,
                     uint8_t use_leds, uint8_t reverse) {

  // draw a bar if reverse is true, we draw right to left vs left to
  // right

  // WARNING: if x+w > screen_width or y+height > screen_height,
  // unpredicable things will happen in memory. There is no protection
  // for overflow here.

  color_t c = Lime;
  float remain_f;

  if (currentval < 0) { currentval = 0; } // never overflow
  if (currentval > maxval) {
    // prevent bar overflow
    remain_f = 1;
  } else {
    remain_f = (float) currentval / (float)maxval;
  }

  int16_t remain = width * remain_f;

#ifdef notdef
  if (use_leds == 1) {
    ledSetFunction(handle_progress);
    ledSetProgress(100 * remain_f);
  }
#endif

  if (remain_f >= 0.8) {
    c = Lime;
  } else if (remain_f >= 0.5) {
    c = Yellow;
  } else {
    c = Red;
  }

  /*
   * Clear the clip variables otherwise
   * gdispGFillArea() might fail.
   */

  GDISP->clipx0 = 0;
	GDISP->clipy0 = 0;
	GDISP->clipx1 = 320;
	GDISP->clipy1 = 240;

  if (reverse) {
    gdispFillArea(x,y+1,(width - remain)-1,height-2, Black);
    gdispFillArea((x+width)-remain,y,remain,height, c);
  } else {
    gdispFillArea(x + remain,y+1,(width - remain)-1,height-2, Black);
    gdispFillArea(x,y,remain,height, c);
  }

  gdispDrawBox(x,y,width,height, c);
}

/*
 * Read a block of pixels from the screen
 * x,y == location of box
 * cx,cy == dimensions of box
 * buf == pointer to pixel buffer
 * Note: buf must be big enough to hold the resulting data
 *       (cx*cy*sizeof(pixel_t))
 */

extern void
getPixelBlock (coord_t x, coord_t y,coord_t cx, coord_t cy, pixel_t * buf)
{
  int i;

  GDISP->p.x = x;
  GDISP->p.y = y;
  GDISP->p.cx = cx;
  GDISP->p.cy = cx;

  gdisp_lld_write_start (GDISP);

  for (i = 0; i < 16*16; i++)
    buf[i] = gdisp_lld_read_color (GDISP);

  gdisp_lld_write_stop (GDISP);

  return;
}

