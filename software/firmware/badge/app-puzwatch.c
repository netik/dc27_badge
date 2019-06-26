/* copied from app-dialer.c */
/* Copyright 2019 New Context Services, Inc. */

#include "ch.h"
#include "hal.h"
#include "hal_spi.h"
#include "chprintf.h"

#include "badge.h"

#include "userconfig.h"

#include "unlocks.h"

#include "orchard-app.h"
#include "ides_gfx.h"
#include "fontlist.h"

#include "ble_lld.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define PUZWAT_PINLEN	4
static int puzwat_running = 0;
static THD_WORKING_AREA(waPuzWThread, 512);

typedef struct puzwat_button {
	coord_t		button_x;
	coord_t		button_y;
	char *		button_text;
} PUZWAT_BUTTON;

#define PUZWAT_MAXBUTTONS 12
#define PUZWAT_EXITBUTTON 9
#define PUZWAT_BSBUTTON 11

static const PUZWAT_BUTTON buttons[] =  {
	{ 30,   60,   "1",    },
	{ 90,  60,   "2",    },
	{ 150, 60,   "3",    },

	{ 30,   120,  "4",    },
	{ 90,  120,  "5",    },
	{ 150, 120,  "6",    },

	{ 30,   180, "7",     },
	{ 90,  180, "8",     },
	{ 150, 180, "9",     },

	{ 30,  240, "Exit", },
	{ 90,  240, "0",     },
	{ 150,  240, "<-", },
};

typedef struct _DHandles {
	/* GListeners */
	GListener		glDListener;

	/* GHandles */
	GHandle			ghButtons[PUZWAT_MAXBUTTONS];
	GHandle			ghPin;

	font_t			font;

	int			pinpos;
	char			pintext[5];

	orientation_t		o;
	uint8_t			sound;
} DHandles;

static THD_FUNCTION(puzwThread, arg)
{

	(void)arg;

	chRegSetThreadName ("PuzWatch");

	while (1) {
		chThdSleepMilliseconds(500);
		if (!puzwat_running && palReadPad (IOPORT1, IOPORT1_SAO_GPIO2) == 0) {
			/* palSetPadMode(IOPORT1, IOPORT1_SAO_GPIO2, PAL_MODE_INPUT_PULLUP); */
			chThdSleepMilliseconds(10);
			if (palReadPad (IOPORT1, IOPORT1_SAO_GPIO2) == 0) {
				orchardAppRun(orchardAppByName("Puzzle Watch"));
			}
		}
	}
}

static uint32_t
puzwat_init(OrchardAppContext *context)
{

	if (context != NULL) {
		/* app start up */
		puzwat_running = 1;
	} else {
		/* required as the pins are set to output on reset */
		palSetPadMode(IOPORT1, IOPORT1_SAO_GPIO1, PAL_MODE_INPUT_PULLUP);
		palSetPadMode(IOPORT1, IOPORT1_SAO_GPIO2, PAL_MODE_INPUT_PULLUP);

		chThdCreateStatic (waPuzWThread, sizeof(waPuzWThread),
		    NORMALPRIO + 1, puzwThread, NULL);
	}

	return (0);
}

static void
draw_screen(OrchardAppContext *context)
{
	DHandles *p;
	GWidgetInit wi;
	const PUZWAT_BUTTON * b;
	int i;

	p = context->priv;

	gwinWidgetClearInit (&wi);

	wi.g.show = TRUE;
	wi.g.width = 60;
	wi.g.height = 60;
	wi.customDraw = gwinButtonDraw_Normal;

	/* Create button widgets */

	wi.customStyle = &DarkGreyStyle;

	for (i = 0; i < PUZWAT_MAXBUTTONS; i++) {
#if 0
		if (i > PUZWAT_EXITBUTON) {
			wi.g.width = 110;
			wi.g.height = 40;
		}
#endif
		b = &buttons[i];
		wi.g.x = b->button_x;
		wi.g.y = b->button_y;
		wi.text = b->button_text;
		p->ghButtons[i] = gwinButtonCreate (0, &wi);
	}

	wi.g.x = 0;
	wi.g.y = 0;
	wi.g.width = 240;
	wi.customDraw = 0;
	p->pinpos = 0;
	strcpy(p->pintext, "____");
	wi.text = p->pintext;
	p->ghPin = gwinLabelCreate(NULL, &wi);

	return;
}

static void destroy_screen(OrchardAppContext *context)
{
	DHandles *p;
	int i;

	p = context->priv;

	for (i = 0; i < PUZWAT_MAXBUTTONS; i++)
		gwinDestroy (p->ghButtons[i]);

	return;
}

static void puzwat_start(OrchardAppContext *context)
{
	DHandles * p;

	p = malloc (sizeof(DHandles));
	memset (p, 0, sizeof(DHandles));
	context->priv = p;
	p->font = gdispOpenFont (FONT_KEYBOARD);

	/*
	 * Clear the screen, and rotate to portrait mode.
	 */

	gdispClear (Black);
	p->o = gdispGetOrientation ();
	gdispSetOrientation (GDISP_ROTATE_0);

	gwinSetDefaultFont (p->font);

	draw_screen (context);

	geventListenerInit(&p->glDListener);
	gwinAttachListener(&p->glDListener);

	geventRegisterCallback (&p->glDListener, orchardAppUgfxCallback,
	    &p->glDListener);

	return;
}

static void
puzwat_unlock_puzzle(void)
{
	userconfig *conf;

	conf = getConfig();
	if (!conf->puz_enabled) {
		conf->puz_enabled = 1;
		configSave(conf);
	}

	chThdSleepMilliseconds(3000);

	NVIC_SystemReset ();
}

static void
puzwat_event(OrchardAppContext *context, const OrchardAppEvent *event)
{
	GEvent * pe;
	DHandles *p;
	int i;

	p = context->priv;

	/* Handle joypad events  */

	if (event->type == keyEvent) {
		if (event->key.flags == keyPress) {
			/* any key to exit */
			orchardAppExit();
		}
	}

	/* Handle uGFX events  */

	if (event->type == ugfxEvent) {

		pe = event->ugfx.pEvent;

		if (pe->type == GEVENT_GWIN_BUTTON) {
			for (i = 0; i < PUZWAT_MAXBUTTONS; i++) {
				if (((GEventGWinButton*)pe)->gwin ==
				    p->ghButtons[i])
					break;
			}
			if (i == PUZWAT_BSBUTTON) {
				if (p->pinpos > 0) {
					p->pinpos--;
					p->pintext[p->pinpos] = '_';
					gwinSetText(p->ghPin, p->pintext, TRUE);
				}
			} else if (i == PUZWAT_EXITBUTTON) {
				orchardAppExit();
			} else {
				p->pintext[p->pinpos++] = buttons[i].button_text[0];
				if (p->pinpos == PUZWAT_PINLEN) {
					/* Check pin */
					gwinSetText(p->ghPin, "Testing...",
					    TRUE);
					if (memcmp((const void *)UL_PUZMODE_PIN,
					    p->pintext, PUZWAT_PINLEN) == 0) {
						gwinSetText(p->ghPin,
						    "Unlocked Puzzle mode!",
						    TRUE);
						puzwat_unlock_puzzle();
					} else
						gwinSetText(p->ghPin,
						    "Incorrect PIN", TRUE);
					p->pinpos = 0;
					strcpy(p->pintext, "____");
				} else
					gwinSetText(p->ghPin, p->pintext, TRUE);
			}
		}
	}

	return;
}

static void
puzwat_exit(OrchardAppContext *context)
{
	DHandles *p;
	p = context->priv;

	puzwat_running = 0;

	destroy_screen (context);

	gdispSetOrientation (p->o);
	gdispCloseFont (p->font);

	geventDetachSource (&p->glDListener, NULL);
	geventRegisterCallback (&p->glDListener, NULL, NULL);

	free (p);
	context->priv = NULL;

	return;
}

/* XXX - graphic for pin pad */
orchard_app("Puzzle Watch", "icons/bell.rgb", APP_FLAG_HIDDEN|APP_FLAG_AUTOINIT, puzwat_init,
             puzwat_start, puzwat_event, puzwat_exit, 9999);
