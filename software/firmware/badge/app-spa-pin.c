/* copied from app-dialer.c */
/* Copyright 2019 New Context Services, Inc. */

#include "orchard-app.h"
#include "ides_gfx.h"
#include "fontlist.h"

#include "ble_lld.h"

#include "chprintf.h"
#include "unlocks.h"
#include "badge.h"


#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define SPAPIN_PINLEN	4

typedef struct spapin_button {
	coord_t		button_x;
	coord_t		button_y;
	char *		button_text;
} SPAPIN_BUTTON;

#define SPAPIN_MAXBUTTONS 12
#define SPAPIN_EXITBUTTON 9
#define SPAPIN_BSBUTTON 11

static const SPAPIN_BUTTON buttons[] =  {
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
	GHandle			ghButtons[SPAPIN_MAXBUTTONS];
	GHandle			ghPin;

	font_t			font;

	int			pinpos;
	char			pintext[SPAPIN_PINLEN + 1];
	char			correctPin[SPAPIN_PINLEN + 1];
	char			correctString[10 + 48 / 4];

	orientation_t		o;
	uint8_t			sound;
} DHandles;

static uint32_t
spapin_init(OrchardAppContext *context)
{
	(void)context;

	/*
	 * We don't want any extra stack space allocated for us.
	 * We'll use the heap.
	 */

	return (0);
}

static void
draw_screen(OrchardAppContext *context)
{
	DHandles *p;
	GWidgetInit wi;
	const SPAPIN_BUTTON * b;
	int i;

	p = context->priv;

	gwinWidgetClearInit (&wi);

	wi.g.show = TRUE;
	wi.g.width = 60;
	wi.g.height = 60;
	wi.customDraw = gwinButtonDraw_Normal;

	/* Create button widgets */

	wi.customStyle = &DarkGreyStyle;

	for (i = 0; i < SPAPIN_MAXBUTTONS; i++) {
#if 0
		if (i > SPAPIN_EXITBUTON) {
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

	for (i = 0; i < SPAPIN_MAXBUTTONS; i++)
		gwinDestroy (p->ghButtons[i]);

	return;
}

static void spapin_start(OrchardAppContext *context)
{
	DHandles * p;

	p = malloc (sizeof(DHandles));
	memset (p, 0, sizeof(DHandles));
	context->priv = p;
	p->font = gdispOpenFont (FONT_KEYBOARD);

	/*
	 * Extract the correct pin.
	 */
#define	UL_PUZPIN_KEY_HIGH	((uint32_t)(UL_PUZPIN_KEY >> 32))
#define	UL_PUZPIN_KEY_LOW	((uint32_t)(UL_PUZPIN_KEY & 0xffffffff))
	snprintf(p->correctPin, sizeof p->correctPin, "%d",
	    (int)(UL_PUZPIN_KEY % 10000));
	snprintf(p->correctString, sizeof p->correctString,
	    "Correct! %04lx%08lx", UL_PUZPIN_KEY_HIGH, UL_PUZPIN_KEY_LOW);

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
spapin_event(OrchardAppContext *context, const OrchardAppEvent *event)
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
			for (i = 0; i < SPAPIN_MAXBUTTONS; i++) {
				if (((GEventGWinButton*)pe)->gwin ==
				    p->ghButtons[i])
					break;
			}
			if (i == SPAPIN_BSBUTTON) {
				if (p->pinpos > 0) {
					p->pinpos--;
					p->pintext[p->pinpos] = '_';
					gwinSetText(p->ghPin, p->pintext, TRUE);
				}
			} else if (i == SPAPIN_EXITBUTTON) {
				orchardAppExit();
			} else {
				p->pintext[p->pinpos++] = buttons[i].button_text[0];
				if (p->pinpos == SPAPIN_PINLEN) {
					/* Check pin */
					gwinSetText(p->ghPin, "Testing...", FALSE);
					for (i = 0; i < SPAPIN_PINLEN; i++) {
						if (p->correctPin[i] == p->pintext[i]) {
							gfxSleepMilliseconds(1000);
						} else
							break;
					}
					if (i == SPAPIN_PINLEN)
						gwinSetText(p->ghPin, p->correctString, FALSE);
					else
						gwinSetText(p->ghPin, "Failure", FALSE);
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
spapin_exit(OrchardAppContext *context)
{
	DHandles *p;
	p = context->priv;

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
orchard_app("Pin Entry", "icons/bell.rgb", 0, spapin_init,
             spapin_start, spapin_event, spapin_exit, 9999);
