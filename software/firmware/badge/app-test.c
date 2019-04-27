#include "orchard-app.h"
#include "orchard-ui.h"

static GListener gl;

static uint32_t
test_init (OrchardAppContext *context)
{
	(void)context;

	/*
	 * We don't want any extra stack space allocated for us.
	 * We'll use the heap.
	 */

	return (0);
}

static void
test_start (OrchardAppContext *context)
{
	font_t font;
	GSourceHandle gs;

	(void)context;

	gdispClear (Blue);

	font = gdispOpenFont ("DejaVuSans24");

	gdispDrawStringBox (0, 0, gdispGetWidth(),
	    gdispGetFontMetric(font, fontHeight),
	    "Hello world......", font, White, justifyCenter);

	gdispCloseFont (font);
 
	font = gdispOpenFont ("fixed_10x20");

	gdispDrawStringBox (0, 40, gdispGetWidth(),
	    gdispGetFontMetric(font, fontHeight),
	    "Test application started", font, White, justifyCenter);

	gdispCloseFont (font);

	gs = ginputGetMouse (0);
	geventListenerInit (&gl);
	geventAttachSource (&gl, gs, GLISTEN_MOUSEMETA);
	geventRegisterCallback (&gl, orchardAppUgfxCallback, &gl);

	return;
}

static void
test_event (OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	GEventMouse * me;

	(void)event;
	(void)context;

	if (event->type == ugfxEvent) {
		me = (GEventMouse *)event->ugfx.pEvent;
		if (me->buttons & GMETA_MOUSE_DOWN) {
			geventRegisterCallback (&gl, NULL, NULL);
			geventDetachSource (&gl, NULL);
			orchardAppExit ();
			return;
		}
	}

	return;
}

static void
test_exit (OrchardAppContext *context)
{
	font_t font;
	(void)context;

	gdispClear (Red);

	font = gdispOpenFont ("fixed_10x20");

	gdispDrawStringBox (0, 40, gdispGetWidth(),
	    gdispGetFontMetric(font, fontHeight),
	    "Test application terminates", font, White, justifyCenter);

	gdispCloseFont (font);

	return;
}

orchard_app("testapp", "icons/compass.rgb",
    0, test_init, test_start, test_event, test_exit, 1);
