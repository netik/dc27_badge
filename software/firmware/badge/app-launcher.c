#include <stdio.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "orchard-app.h"
#include "fontlist.h"
#include "ides_gfx.h"
#include "src/gdisp/gdisp_driver.h"
#include "src/gwin/gwin_class.h"
#include "userconfig.h"
#include "i2s_lld.h"
#include "ble_lld.h"

#define LAUNCHER_COLS 3
#define LAUNCHER_ROWS 2
#define LAUNCHER_PERPAGE (LAUNCHER_ROWS * LAUNCHER_COLS)

extern const OrchardApp *orchard_app_list;
static uint32_t last_ui_time;

static unsigned int selected = 0;
static unsigned int page = 0;

struct launcher_list_item {
	const char *		name;
	const OrchardApp *	entry;
};

struct launcher_list {
	font_t			fontLG;
	font_t			fontXS;
	font_t			fontSM;
	GHandle			ghTitleL;
	GHandle			ghTitleR;
	GHandle			ghButtonUp;
	GHandle			ghButtonDn;
	/* app grid */
	GHandle			ghButtons[6];
	GHandle			ghLabels[6];

	GListener		gl;

	unsigned int		total;

	struct launcher_list_item items[0];
};

static void redraw_list(struct launcher_list *list);

static void
draw_launcher_buttons(struct launcher_list * list)
{
	GWidgetInit wi;
	unsigned int i,j;
	char tmp[20];
	userconfig *config = getConfig();

	gwinWidgetClearInit (&wi);

	/* Create label widget: ghTitleL */
	wi.g.show = TRUE;
	wi.g.x = 0;
	wi.g.y = 0;
	wi.g.width = 140;
	wi.g.height = -1;
	wi.text = config->name;
	wi.customDraw = gwinLabelDrawJustifiedLeft;
	wi.customParam = 0;
	wi.customStyle = &DarkPurpleStyle;
	list->ghTitleL = gwinLabelCreate(0, &wi);
	gwinLabelSetBorder (list->ghTitleL, FALSE);
	gwinSetFont (list->ghTitleL, list->fontSM);
	gwinRedraw (list->ghTitleL);

	/* Create label widget: ghTitleR */
	snprintf (tmp, sizeof(tmp), "%x:%x:%x:%x:%x:%x",
	    ble_station_addr[0], ble_station_addr[1], ble_station_addr[2],
	    ble_station_addr[3], ble_station_addr[4], ble_station_addr[5]);

	wi.g.show = TRUE;
	wi.g.x = 141;
	wi.g.width = 170;
	wi.g.height = -1;
	wi.text = tmp;
	wi.customDraw = gwinLabelDrawJustifiedRight;
	list->ghTitleR = gwinLabelCreate (0, &wi);
	gwinSetFont (list->ghTitleR, list->fontSM);
	gwinLabelSetBorder (list->ghTitleR, FALSE);
	gwinRedraw (list->ghTitleR);

	/* draw the button grid
	 *
	 * buttons
	 * xloc: 0, 90, 180
	 * yloc: 30, 140
	 *
	 * label
	 * xloc: 0, 90, 180
	 * yloc: 110, 220
	 */

	/*
	 * Each button in the launcher grid is a transparent area that we
	 * draw the image onto after the fact using redraw_list. We cannot
	 * use standard ugfx button rendering here because we are severly
	 * ram-constrained. we must put images and then close them as soon
	 * as possible
	 */

	gwinSetDefaultFont(list->fontXS);

	for (i = 0; i < LAUNCHER_ROWS; i++) {
		for (j = 0; j < LAUNCHER_COLS; j++) {
			wi.g.show = TRUE;
			wi.g.x = (j * 90) + 2;
			wi.g.y = 31 + (110 * i);
			wi.g.width = 80;
			wi.g.height = 80;
			wi.text = "";
			wi.customDraw = noRender;
			wi.customParam = 0;
			wi.customStyle = 0;
			list->ghButtons[(i * LAUNCHER_COLS) + j] =
			    gwinButtonCreate (0, &wi);

			/* Create label widget: ghLabel1 */

			wi.g.x = (j * 90) + 2;
			wi.g.y = 110 * (i+1);
			wi.g.height = 20;
			wi.text = "---";
			wi.customDraw = gwinLabelDrawJustifiedCenter;
			list->ghLabels[(i * LAUNCHER_COLS) + j] =
			    gwinLabelCreate (0, &wi);
		}
	}

	/* create button widget: ghButtonUp */

	wi.g.show = TRUE;
	wi.g.x = 270;
	wi.g.y = 30;
	wi.g.width = 50;
	wi.g.height = 50;
	wi.text = "";
	wi.customDraw = gwinButtonDraw_ArrowUp;
	wi.customParam = 0;
	wi.customStyle = &DarkPurpleStyle;
	list->ghButtonUp = gwinButtonCreate (0, &wi);

	/* create button widget: ghButtonDn */

	wi.g.y = 190;
	wi.customDraw = gwinButtonDraw_ArrowDown;
	list->ghButtonDn = gwinButtonCreate (0, &wi);
	gwinRedraw (list->ghButtonDn);

	redraw_list(list);

	return;
}

static void
draw_box (struct launcher_list * list, color_t color)
{
	unsigned int i,j;
        unsigned int x, y;

	/* row y */

	i = (selected % LAUNCHER_PERPAGE) / LAUNCHER_COLS;

	/* col x */

	j = selected % LAUNCHER_COLS;

	_gwinDrawStart (list->ghButtonDn);

	/* Clear the clip variables otherwise gdispGFillArea() might fail. */

	GDISP->clipx0 = 0;
	GDISP->clipy0 = 0;
	GDISP->clipx1 = gdispGetWidth ();
	GDISP->clipy1 = gdispGetHeight ();

	x = (j * 90) + 2;
	y = 30 + (110 * i);

	x -= 1;
	y -= 1;

	gdispDrawBox (x, y, 82, 82, color);

	_gwinDrawEnd (list->ghButtonDn);

	return;
}

static void
redraw_list (struct launcher_list * list)
{
	unsigned int i, j;
	unsigned int actualid;
	struct launcher_list_item * item;
	GHandle label;

	/*
	 * given a page number, put down an image for each of the buttons
	 * and set the label.
	 */

	for (i = 0; i < LAUNCHER_ROWS; i++) {
		for (j = 0; j < LAUNCHER_COLS; j++) {
			actualid = (page * LAUNCHER_PERPAGE) +
			    (i * LAUNCHER_COLS) + j;
	 		label = list->ghLabels[(i * LAUNCHER_COLS) + j];
			item = &list->items[actualid];

			_gwinDrawStart (list->ghButtonDn);

			/*
			 * Clear the clip variables otherwise
			 * gdispGFillArea() might fail.
			 */

			GDISP->clipx0 = 0;
			GDISP->clipy0 = 0;
			GDISP->clipx1 = 320;
			GDISP->clipy1 = 240;

			if (actualid < list->total) {
				gwinSetText (label, item->name, FALSE);
				if (item->entry->icon != NULL) {
					putImageFile (item->entry->icon,
					    (j * 90) + 2, 30 + (110 * i));
				}
			} else {
				gwinSetText (label, "", FALSE);
				gdispFillArea ((j * 90) + 2, 30 + (110 * i),
				    81, 81, Black);
			}

			_gwinDrawEnd (list->ghButtonDn);
		}
	}


	draw_box (list, Red);

	return;
}

static uint32_t
launcher_init (OrchardAppContext *context)
{
	(void)context;

	gdispClear (Black);

	return (0);
}

static void
launcher_start (OrchardAppContext *context)
{
	struct launcher_list *list;
	const OrchardApp *current;
	unsigned int total_apps = 0;
	userconfig *config = getConfig();

	/* How many apps do we have? */
	current = orchard_app_list;
	while (current->name) {
		if ((current->flags & APP_FLAG_HIDDEN) == 0)
			total_apps++;
		current++;
	}

	list = malloc (sizeof(struct launcher_list) +
	    + (total_apps * sizeof(struct launcher_list_item)) );

	context->priv = list;

	list->fontXS = gdispOpenFont (FONT_XS);
	list->fontLG = gdispOpenFont (FONT_LG);
	list->fontSM = gdispOpenFont (FONT_SM);

	/* Rebuild the app list */
	current = orchard_app_list;
	list->total = 0;
	while (current->name) {
		if ((current->flags & APP_FLAG_HIDDEN) == 0) {
			list->items[list->total].name = current->name;
			list->items[list->total].entry = current;
			list->total++;
		}
		current++;
 	}

	draw_launcher_buttons (list);

	/* set up our local listener */
	geventListenerInit (&list->gl);
	gwinAttachListener (&list->gl);
	geventRegisterCallback (&list->gl, orchardAppUgfxCallback, &list->gl);

	/* set up our idle timer */
	orchardAppTimer (context, 1000000, true);
	last_ui_time = 0;

	/* forcibly run the name app if our name is blank. Sorry. */

	if (strlen(config->name) == 0) {
		orchardAppRun (orchardAppByName ("Set your name"));
		return;
	}
#ifdef notdef
	/* if you don't have a type set, force that too. */

	if (config->p_type == 0) {
		orchardAppRun (orchardAppByName ("Choosetype"));
		return;
	}
#endif

	return;
}

static void
launcher_event (OrchardAppContext *context, const OrchardAppEvent *event)
{
	GEvent * pe;
	struct launcher_list * list;
	unsigned int i;
	GHandle w;
	unsigned int currapp;

	/* Ignore radio events */
	if (event->type == radioEvent)
		return;

	/*
	 * Timer events trigger once a second. If we get ten timer
	 * events with no other events in between, then we time out
	 * back to the main status screen.
	 */

	if (event->type == timerEvent) {
		last_ui_time++;
		if (last_ui_time == UI_IDLE_TIME) {
    			orchardAppRun (orchardAppByName ("Badge"));
		}
    		return;
  	}

	/* Reset the timeout count */

	last_ui_time = 0;

	list = (struct launcher_list *)context->priv;

	if (event->type == keyEvent && event->key.flags == keyRelease)
		return;

	if (event->type == keyEvent) {

		if (event->key.flags == keyPress) {
			if (event->key.code == keyBSelect) {
				i2sPlay ("sound/ping.snd");
			} else {
				i2sPlay ("sound/click.snd");
			}
		}

		draw_box (list, Black);

		if (event->key.code == keyAUp || event->key.code == keyBUp)
			selected -= LAUNCHER_COLS;

		if (event->key.code == keyADown ||
		    event->key.code == keyBDown) {
			if (selected + LAUNCHER_COLS <=
			   (list->total - 1))
				selected += LAUNCHER_COLS;
		}

		if (event->key.code == keyALeft ||
		    event->key.code == keyBLeft) {
			if (selected > 0)
				selected--;
		}

		if (event->key.code == keyARight ||
		    event->key.code == keyBRight) {
			if (selected < (list->total - 1))
				selected++;
		}

		if (selected > 250)
			selected  = 0;

		if (selected > list->total)
			selected = list->total;

		if (selected >= ((page + 1) * LAUNCHER_PERPAGE)) {
			page++;
			redraw_list (list);
			return;
		}

		if (page > 0) {
			if (selected < (page * LAUNCHER_PERPAGE)) {
				page--;
				redraw_list (list);
				return;
			}
		}

		if (event->key.code == keyBSelect) {
			orchardAppRun (list->items[selected].entry);
			return;
		}

		draw_box (list, Red);
		return;
	}

	pe = event->ugfx.pEvent;

	if (event->type == ugfxEvent && pe->type == GEVENT_GWIN_BUTTON) {

		w = ((GEventGWinButton*)pe)->gwin;
		draw_box (list, Black);

		for (i = 0; i < 6; i++) {
			currapp = (page * LAUNCHER_PERPAGE) + i;
			if (w == list->ghButtons[i] &&
			    currapp < list->total) {
				i2sPlay ("sound/ping.snd");
				orchardAppRun (list->items[currapp].entry);
				return;
			}
		}

		i2sPlay ("sound/click.snd");

		/*
		 * only update the location if we're still
		 * within a valid page range
		 */

		if (w == list->ghButtonDn) {
			if (((page + 1) * LAUNCHER_PERPAGE) <
			    list->total) {
				/* remove the box before update  */
				selected += LAUNCHER_PERPAGE;
				if (selected > (list->total - 1))
					selected = (list->total - 1);
				page++;
				redraw_list (list);
				return;
			}
		}

		if (w == list->ghButtonUp) {
			if (page > 0) {
				/* remove the box before update  */
				page--;
				selected -= LAUNCHER_PERPAGE;
				redraw_list (list);
				return;
			}
		}
		draw_box (list, Red);
	}

	/* wraparound */

	if (selected > 255) // underflow
		selected = list->total-1;

	if (selected >= list->total)
		selected = 0;

	return;
}

static void
launcher_exit (OrchardAppContext *context)
{
	struct launcher_list *list;
	unsigned int i;

	list = (struct launcher_list *)context->priv;

	gdispCloseFont (list->fontXS);
	gdispCloseFont (list->fontLG);
	gdispCloseFont (list->fontSM);

	gwinDestroy (list->ghTitleL);
	gwinDestroy (list->ghTitleR);
	gwinDestroy (list->ghButtonUp);
	gwinDestroy (list->ghButtonDn);

	/* nuke the grid */
	for (i = 0; i < 6; i++) {
		gwinDestroy (list->ghButtons[i]);
		gwinDestroy (list->ghLabels[i]);
	}

	geventRegisterCallback (&list->gl, NULL, NULL);
	geventDetachSource (&list->gl, NULL);

	free (context->priv);
	context->priv = NULL;

	return;
}

/* the app labelled as app_1 is always the launcher. changing this
   section header will move the code further down the list and will
   cause a different app to become the launcher -- jna */

const OrchardApp _orchard_app_list_launcher
__attribute__((used, aligned(4), section(".chibi_list_app_1_launcher"))) = {
  "Launcher",
  NULL,
  APP_FLAG_HIDDEN,
  launcher_init,
  launcher_start,
  launcher_event,
  launcher_exit,
};
