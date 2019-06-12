#ifndef __IDES_GFX_H__
#define __IDES_GFX_H__

/* ides_gfx.h
 *
 * Shared Graphics Routines
 * J. Adams <1/2017>
 */

extern const GWidgetStyle RedButtonStyle;
extern const GWidgetStyle DarkPurpleStyle;
extern const GWidgetStyle DarkPurpleFilledStyle;
extern const GWidgetStyle IvoryStyle;

/* Graphics */
extern int putImageFile (char *name, int16_t x, int16_t y);
extern void drawProgressBar(coord_t x, coord_t y, coord_t width, coord_t height, int32_t maxval, int32_t currentval, uint8_t use_leds, uint8_t reverse);
extern int putImageFile(char *name, int16_t x, int16_t y);
extern void blinkText (coord_t x, coord_t y,coord_t cx, coord_t cy, char *text, font_t font, color_t color, justify_t justify, uint8_t times, int16_t delay);
extern char *getAvatarImage(int ptype, char *imgtype, uint8_t frameno, uint8_t reverse);
extern void screen_alert_draw(uint8_t clear, char *msg);
extern void getPixelBlock (coord_t x, coord_t y,coord_t cx,
    coord_t cy, pixel_t * buf);
extern void putPixelBlock (coord_t x, coord_t y,coord_t cx,
    coord_t cy, pixel_t * buf);
extern void drawBufferedStringBox(
      pixel_t **fb,
      coord_t x,
      coord_t y,
      coord_t cx,
      coord_t cy,
      const char* str,
      font_t font,
      color_t color,
      justify_t justify);
#endif /* __IDES_GFX_H__ */

/* used by screen_alert_draw */
#define ALERT_DELAY_SHORT 1500
#define ALERT_DELAY  3000
