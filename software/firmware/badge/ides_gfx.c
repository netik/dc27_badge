
/* ides_gfx.c
 *
 * Shared Graphics Routines
 * J. Adams (dc25: 1/2017, dc27: 6/2019)
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

#include "battle.h"

#include <stdio.h>

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

const GWidgetStyle GreenButtonStyle = {
    HTML2COLOR(0x00ff00),          // background
    HTML2COLOR(0x66ff66),          // focus

  // Enabled color set
  {
    HTML2COLOR(0xffffff),         // text
    HTML2COLOR(0x008000),         // edge
    HTML2COLOR(0x00ff00),         // fill
    HTML2COLOR(0x800000),         // progress (inactive area)
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
    HTML2COLOR(0x008000),         // edge
    HTML2COLOR(0x6aff71),         // fill
    HTML2COLOR(0x800000),         // progress (active area)
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
const GWidgetStyle DarkGreyStyle = {
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

static int
putGifImage (char *name, int16_t x, int16_t y)
{
	gdispImage img;

	if (gdispImageOpenFile (&img, name) == GDISP_IMAGE_ERR_OK) {
		gdispImageDraw (&img, x, y, img.width, img.height, 0, 0);
		gdispImageClose (&img);
		return (1);
	}

	return (0);
}

static int
putRgbImage (char *name, int16_t x, int16_t y)
{
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

	return (0);
}

int
putImageFile (char *name, int16_t x, int16_t y)
{
	int r;

	if (strstr (name, ".gif") != NULL)
		r = putGifImage (name, x, y);
	else
		r = putRgbImage (name, x, y);

	return (r);
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

	gdispDrawThickLine (0, middle - 20, 320, middle -20, Blue, 2, FALSE);
	gdispDrawThickLine (0, middle + 20, 320, middle +20, Blue, 2, FALSE);

	gdispDrawStringBox (0,
	    middle - (gdispGetFontMetric(fontFF, fontHeight) >> 1),
	    gdispGetWidth(), gdispGetFontMetric(fontFF, fontHeight),
	    msg, fontFF, Yellow, justifyCenter);


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
  int16_t remain;
  float remain_f;

  if (currentval < 0)
    currentval = 0; // never overflow

  if (currentval > maxval) {
    // prevent bar overflow
    remain_f = 1;
  } else {
    remain_f = (float)currentval / (float)maxval;
  }

  remain = width * remain_f;

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

  gdispDrawBox (x, y, width, height, c);
  gdispDrawBox (x - 1, y - 1, width + 2, height + 2, Black);
}

/*
 * Read a block of pixels from the screen
 * x,y == location of box
 * cx,cy == dimensions of box
 * buf == pointer to pixel buffer
 * Note: buf must be big enough to hold the resulting data
 *       (cx x cy x sizeof(pixel_t))
 * Reading pixels from the display is a bit painful on the ILI9341, because
 * it returns 24-bit RGB color data instead of 16-bit RGB565 pixel data.
 * The gdisp_lld_read_color() does conversion in software.
 */

void
getPixelBlock (coord_t x, coord_t y, coord_t cx, coord_t cy, pixel_t * buf)
{
  int i;

  GDISP->p.x = x;
  GDISP->p.y = y;
  GDISP->p.cx = cx;
  GDISP->p.cy = cy;

  gdisp_lld_read_start (GDISP);

  for (i = 0; i < (cx * cy); i++)
    buf[i] = gdisp_lld_read_color (GDISP);

  gdisp_lld_read_stop (GDISP);

  return;
}

/*
 * Write a block of pixels to the screen
 * x,y == location of box
 * cx,cy == dimensions of box
 * buf == pointer to pixel buffer
 * Note: buf must be big enough to hold the resulting data
 *       (cx*cy*sizeof(pixel_t))
 * This is basically a blitter function, but it's a little more
 * efficient than the one in uGFX. We bypass the gdisp_lld_write_color()
 * function and dump the pixels straight into the SPI bus.
 * Note: Pixel data must be in big endian form, since the Nordic SPI
 * controller doesn't support 16-bit accesses (like the NXP one did).
 * This means we're really using RGB565be format.
 */

void
putPixelBlock (coord_t x, coord_t y, coord_t cx, coord_t cy, pixel_t * buf)
{
  GDISP->p.x = x;
  GDISP->p.y = y;
  GDISP->p.cx = cx;
  GDISP->p.cy = cy;

  gdisp_lld_write_start (GDISP);

  spiSend (&SPID4, cx * cy * sizeof(pixel_t), buf);

  gdisp_lld_write_stop (GDISP);

  return;
}

// this allows us to write text to the screen and remember the background
void drawBufferedStringBox(
  pixel_t **fb,
  coord_t x,
  coord_t y,
  coord_t cx,
  coord_t cy,
  const char* str,
  font_t font,
  color_t color,
  justify_t justify) {
    // if this is the first time through, init the buffer and
    // remember the background
    if (*fb == NULL) {
      *fb = (pixel_t *) malloc(cx * cy * sizeof(pixel_t));
      // get the pixels
      getPixelBlock (x, y, cx, cy, *fb);
    } else {
      // paint it back.
      putPixelBlock (x, y, cx, cy, *fb);
    }

    // paint the text
    gdispDrawStringBox(x,y,cx,cy,str,font,color,justify);
}

char *getAvatarImage(int shipclass, bool is_player, char frame, bool is_right) {
  static char fname[64];

  /*
   * Ships are identified by the following filename structure: game/ + ...
   *   s2e-h-l.tif
   *    ^^ ^ ^
   *    || | +---- Orientation (L)eft facing or (R)ight facing
   *    || +------ Frame Type (see below)
   *    |+-------- 'e' enemy or 'p' player. Basically, Red, or Black.
   *    +--------- Ship class number (see ships.c/ships.h)
   *    Ship class numbers
   *
   * 0:   PT_Boat
   * 1:   Patrol_Ship
   * 2:   Destroyer
   * 3:   ruiser
   * 4:   Frigate
   * 5:   Battleship
   * 6:   Submarine
   * 7:   Tesla_Ship
   *
   * Frame type
   * This indicates which animation frame this image is for.
   *
   * n    normal
   * h    ship was hit
   * d    ship was destroyed
   * g    ship is regenerating
   * t    ship is teleporting
   * s    ship has shields up
   * u    (u-boat) ship is submerged
   *
   */

  sprintf(fname,
          "game/s%d%c-%c-%c.rgb",
          shipclass,
          is_player ? 'p' : 'e',
          frame,
          is_right ? 'r' : 'l');

  return(fname);
}

color_t levelcolor(uint16_t player_level, uint16_t enemy_level) {
  color_t color = HTML2COLOR(0x7efb03);
  int16_t leveldelta;

  leveldelta = enemy_level - player_level;
  // range:
  //
  //  >=  3 = 2x XP     RED
  //  >=  2 = 1.50x XP  ORG
  //  >=  1 = 1.25x XP  YEL
  //  ==  0 = 1x XP     GREEN
  //  <= -1 = .75x XP   GREY
  //  <= -2 = .25x XP   GREY
  //  <= -3 = 0 XP      GREY

  // you killed someone harder than you
  if (leveldelta >= 3)
    color = HTML2COLOR(0xff00ff);
  else
    if (leveldelta >= 2)
      color = HTML2COLOR(0xf1a95b);
    else
      if (leveldelta >= 1)
        color = HTML2COLOR(0xf6ff00);

  // you killed someone weaker than you
  if (leveldelta <= -1)
    color = Grey;

  return color;
}
