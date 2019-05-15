#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "orchard-app.h"
#include "orchard-ui.h"
#include "fontlist.h"
#include "i2s_lld.h"
#include "ides_gfx.h"
#include "gll.h"
#include "userconfig.h"

/* private memory */
typedef struct {
	GListener		gl;
	font_t			fontLG;
	font_t			fontXS;
	font_t			fontSM;
} test_ui_t;

typedef int16_t joypad_test_t;

#define JOY_UP     1UL << 1
#define JOY_DOWN   1UL << 2
#define JOY_LEFT   1UL << 3
#define JOY_RIGHT  1UL << 4
#define JOY_SELECT 1UL << 5

joypad_test_t joya = 0;
joypad_test_t joyb = 0;
joypad_test_t joya_tested = 0;
joypad_test_t joyb_tested = 0;

extern void splash_footer(void);

static uint32_t test_init(OrchardAppContext *context)
{
	(void)context;

	/*
	 * We don't want any extra stack space allocated for us.
	 * We'll use the heap.
	 */

	return (0);
}

static void drawJoyBox(joypad_test_t j, coord_t offsetx, color_t oncolor) {
	const coord_t radius = 20;

	// draws a joystick in a 160px x 160 px box
	// left
	gdispDrawCircle(offsetx + 40, 120, radius, White);

	// mid
	gdispDrawCircle(offsetx + 80, 120, radius, White);

	// r
	gdispDrawCircle(offsetx + 120, 120, radius, White);

	// up
	gdispDrawCircle(offsetx + 80, 80, radius, White);

	// down
	gdispDrawCircle(offsetx + 80, 160, radius, White);

	// make them all Green ------------------------------
	if (j & JOY_LEFT)
  	gdispFillCircle(offsetx + 40, 120, radius / 2, oncolor);

	// mid
	if (j & JOY_SELECT)
   	gdispFillCircle(offsetx + 80, 120, radius / 2, oncolor);

	// r
	if (j & JOY_RIGHT)
  	gdispFillCircle(offsetx + 120, 120, radius / 2, oncolor);

	// up
	if (j & JOY_UP)
  	gdispFillCircle(offsetx + 80, 80, radius / 2, oncolor);

	// down
	if (j & JOY_DOWN)
  	gdispFillCircle(offsetx + 80, 160, radius / 2, oncolor);

}

static void draw_test_ui(OrchardAppContext *context) {
	test_ui_t *th;
	coord_t ypos;
  // get private memory
  th = (test_ui_t *)context->priv;

	// top
	ypos = 0 ;
	gdispDrawStringBox (0,
					0,
					320,
					gdispGetFontMetric(th->fontSM, fontHeight),
					"TEST MODE",
					th->fontSM, Cyan, justifyCenter);
					ypos = ypos + gdispGetFontMetric(th->fontSM, fontHeight) + 2;

	gdispDrawStringBox (0,
					ypos,
					320,
					gdispGetFontMetric(th->fontSM, fontHeight),
					"HIT RESET TO EXIT",
					th->fontSM, Blue, justifyCenter);

	drawJoyBox(joya, 0, White);
	drawJoyBox(joyb, 160, White);

	// versioning
	splash_footer();
}


static void test_start(OrchardAppContext *context)
{
	test_ui_t *th;
	th = malloc(sizeof(test_ui_t));
	context->priv = th;
	th->fontXS = gdispOpenFont (FONT_XS);
	th->fontLG = gdispOpenFont (FONT_LG);
	th->fontSM = gdispOpenFont (FONT_SM);

	gdispClear(Black);
	draw_test_ui(context);
	return;
}

static void test_event(OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	if (event->type == keyEvent) {
		if (event->key.flags == keyPress)  {
			switch (event->key.code) {
				case keyASelect:
				  joya |= JOY_SELECT;
			  	joya_tested |= JOY_SELECT;
			  	break;
				case keyAUp:
					joya |= JOY_UP;
					joya_tested |= JOY_UP;
					break;
				case keyADown:
					joya |= JOY_DOWN;
					joya_tested |= JOY_DOWN;
					break;
				case keyALeft:
					joya |= JOY_LEFT;
					joya_tested |= JOY_LEFT;
					break;
				case keyARight:
					joya |= JOY_RIGHT;
					joya_tested |= JOY_RIGHT;
					break;
				case keyBSelect:
					joyb |= JOY_SELECT;
					joyb_tested |= JOY_SELECT;
					break;
				case keyBUp:
					joyb |= JOY_UP;
					joyb_tested |= JOY_UP;
					break;
				case keyBDown:
					joyb |= JOY_DOWN;
					joyb_tested |= JOY_DOWN;
					break;
				case keyBLeft:
					joyb |= JOY_LEFT;
					joyb_tested |= JOY_LEFT;
					break;
				case keyBRight:
					joyb |= JOY_RIGHT;
					joyb_tested |= JOY_RIGHT;
					break;
				default:
					break;
			}
		}

		if (event->key.flags == keyRelease) {
			switch (event->key.code) {
				case keyASelect:
					joya &= ~(JOY_SELECT);
					break;
				case keyAUp:
					joya &= ~(JOY_UP);
					break;
				case keyADown:
					joya &= ~(JOY_DOWN);
					break;
				case keyALeft:
					joya &= ~(JOY_LEFT);
					break;
				case keyARight:
					joya &= ~(JOY_RIGHT);
					break;
				case keyBSelect:
					joyb &= ~(JOY_SELECT);
					break;
				case keyBUp:
					joyb &= ~(JOY_UP);
					break;
				case keyBDown:
					joyb &= ~(JOY_DOWN);
					break;
				case keyBLeft:
					joyb &= ~(JOY_LEFT);
					break;
				case keyBRight:
					joyb &= ~(JOY_RIGHT);
					break;
				default:
					break;
			}
		}

		drawJoyBox(joya_tested, 0, Green);
		drawJoyBox(joyb_tested, 160, Green);

		drawJoyBox(joya, 0, White);
		drawJoyBox(joyb, 160, White);
	}
	return;
}

static void test_exit(OrchardAppContext *context)
{
	test_ui_t *th;
	th = context->priv;

	gdispCloseFont (th->fontXS);
	gdispCloseFont (th->fontLG);
	gdispCloseFont (th->fontSM);

	free(th);
	context->priv = NULL;
	return;
}

orchard_app("Joypad Test", "icons/ship.rgb", 0,
  test_init, test_start, test_event, test_exit, 9999);
