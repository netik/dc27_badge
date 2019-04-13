#ifndef __IDES_GFX_H__
#define __IDEX_GFX_H__

/* ides_gfx.h
 *
 * Shared Graphics Routines
 * J. Adams <1/2017>
 */

extern const GWidgetStyle RedButtonStyle;
extern const GWidgetStyle DarkPurpleStyle;
extern const GWidgetStyle DarkPurpleFilledStyle;
extern const GWidgetStyle IvoryStyle;

extern int putImageFile (char *name, int16_t x, int16_t y);
extern void screen_alert_draw (uint8_t clear, char *msg);

#endif /* __IDEX_GFX_H__ */
