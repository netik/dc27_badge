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

