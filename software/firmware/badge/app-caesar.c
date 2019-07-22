/* Copyright 2019 New Context Services, Inc. */

#include "orchard-app.h"
#include "ides_gfx.h"
#include "fontlist.h"

#include "ble_lld.h"

#include "chprintf.h"
#include "unlocks.h"
#include "badge.h"
#include "userconfig.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

size_t strlcpy(char * __restrict dst, const char * __restrict src, size_t dsize);


struct caesar_buttons {
	coord_t		cb_x1;		/* upper left corner */
	coord_t		cb_y1;
	coord_t		cb_x2;		/* lower right corner, inclusive */
	coord_t		cb_y2;
	char *		cb_text;	/* initial text */
};

/* mapping from meaning to possition */
#define CB_INPUT	0
#define CB_KEY		1
#define CB_OUTPUT	2
#define CB_SWAP		3
#define CB_CAESAR	4
#define CB_ENCIPHER	5
#define CB_DECIPHER	6
#define CB_EXIT		7
#define CB_NBUTTONS	8
static const char *casear_keyboarduitext[] = {
	[CB_INPUT] = "Enter input text, max 80 characters.",
	[CB_KEY] = "Enter key stream values.\nValue 0 through 25, separated\nby spaces, or a slash.",
	[CB_CAESAR] = "Enter rotation value.\nValue 0 through 25.",
};

static const char special_message[] = "jusnlplntafaiibeexcmbdj";

static const struct caesar_buttons caesar_buttons[] = {
	{   0,   0, 259,  59, "Input Text" },
	{   0,  60, 259, 119, "Key" },
	{   0, 120, 259, 179, "Output" },
	{ 260,   0, 319, 179, "Swap" },
	{   0, 180,  69, 239, "Caesar" },
	{  70, 180, 159, 239, "Encipher" },
	{ 160, 180, 249, 239, "Decipher" },
	{ 250, 180, 319, 239, "Exit" },
};

#define	CB_MAXLEN 81
#define	CB_KEYMAXLEN 160
struct caesar_data {
	GListener	cb_listener;

	int		cb_buttons_initd;
	GHandle		cb_buttons[CB_NBUTTONS];
	int		cb_outputshown;
	GHandle		cb_outputbut;
	font_t		font;

	orientation_t	o;

	/* keyboard */
	struct OrchardUiContext	cb_uicontext;
	const char	*cb_itemlist[2];
	char		cb_keyboardinp[CB_KEYMAXLEN];
	int		cb_inpbutton;			/* button that trigger keyboard */

	/* data */
	char		cb_input[CB_MAXLEN];		/* NUL terminated */
	uint8_t		cb_keystream[CB_MAXLEN];	/* end denoted by >=26 */
	char		cb_output[CB_MAXLEN];		/* NUL terminated */
};

static int
min(int a, int b)
{
	if (a < b)
		return a;

	return b;
}

static uint32_t
caesar_init(OrchardAppContext *context)
{
	(void)context;

	return (0);
}

static void
keystream_to_str(const uint8_t *key, char *out, int len)
{
	int i;
	char *sep = "";

	for (i = 0; len > 1 && key[i] < 26; i++) {
		snprintf(out, len, "%s%d", sep, key[i]);

		len -= strlen(out);
		out += strlen(out);

		sep = " ";
	}
}

static void
str_to_keystream(const char *inp, uint8_t *key, int len)
{
	int i;
	long val;
	char *end;

	printf("s2k: %p, %s\n", inp, inp);
	for (i = 0; i < len;) {
		val = strtol(inp, &end, 10);
		printf("c: %ld, %p\n", val, end);
		if (inp == end) {
			if (*end == '/') {
				inp = &end[1];
				continue;
			}
			val = 26;
		}

		if (val > 25 || val < 0)
			val = 26;
		key[i++] = val;
		if (val == 26)
			break;

		inp = end;
	}

	printf("e: %d\n", i);

	if (key[0] == 26) {
		key[0] = 13;
		key[1] = 26;
	}
}

static void
draw_output(OrchardAppContext *context)
{
	struct caesar_data *p;
	char *str, *pos, *strpos;
	GWidgetInit wi;
	int i;

	p = context->priv;

	if (p->cb_outputshown)
		return;

	gwinWidgetClearInit(&wi);

	wi.g.show = TRUE;
	wi.customDraw = gwinButtonDraw_Normal;

	/* Create button widgets */
	wi.customStyle = &DarkPurpleStyle;

	wi.g.x = 0;
	wi.g.y = 0;;
	wi.g.width = 320;
	wi.g.height = 240;
	wi.text = "";
	p->cb_outputbut = gwinButtonCreate (0, &wi);

#define CB_CHARSPERLINE 20
	i = CB_MAXLEN + (CB_MAXLEN + CB_CHARSPERLINE - 1) / CB_CHARSPERLINE;
	printf("i: %d, %d, %d\n", i, CB_MAXLEN, CB_CHARSPERLINE);
	str = malloc(i + 1);
	strpos = str;
	pos = p->cb_output;
	while (*pos && i) {
		if (strpos != str) {
			strcat(strpos++, "\n");
			i--;
		}
		printf("min: %d\n", min(i, CB_CHARSPERLINE + 1));
		strlcpy(strpos, pos, min(i, CB_CHARSPERLINE + 1));
		pos += strlen(strpos);
		i -= strlen(strpos);
		strpos += strlen(strpos);
	}

	gwinSetText(p->cb_outputbut, str, TRUE);
	free(str);

	p->cb_outputshown = 1;

	geventListenerInit(&p->cb_listener);
	gwinAttachListener(&p->cb_listener);

	geventRegisterCallback (&p->cb_listener, orchardAppUgfxCallback,
	    &p->cb_listener);

	return;
}

static void
destroy_output(OrchardAppContext *context)
{
	struct caesar_data *p;

	p = context->priv;

	if (!p->cb_outputshown)
		return;

        geventRegisterCallback (&p->cb_listener, NULL, NULL);
        geventDetachSource (&p->cb_listener, NULL);

	gwinDestroy(p->cb_outputbut);

	p->cb_outputshown = 0;
}

static void
draw_screen(OrchardAppContext *context)
{
	struct caesar_data *p;
	GWidgetInit wi;
	const struct caesar_buttons *b;
	char *keystr;
	int i;

	keystr = NULL;
	p = context->priv;

	if (p->cb_buttons_initd)
		return;

	gwinWidgetClearInit(&wi);

	wi.g.show = TRUE;
	wi.customDraw = gwinButtonDraw_Normal;

	/* Create button widgets */
	wi.customStyle = &DarkPurpleStyle;

	for (i = 0; i < CB_NBUTTONS; i++) {
		b = &caesar_buttons[i];
		wi.g.x = b->cb_x1;
		wi.g.y = b->cb_y1;
		wi.g.width = b->cb_x2 - b->cb_x1 + 1;
		wi.g.height = b->cb_y2 - b->cb_y1 + 1;
		wi.text = b->cb_text;
		p->cb_buttons[i] = gwinButtonCreate (0, &wi);
		if (i == CB_INPUT)
			gwinSetText(p->cb_buttons[i], p->cb_input, FALSE);
		else if (i == CB_KEY) {
			keystr = malloc(CB_KEYMAXLEN);
			keystream_to_str(p->cb_keystream, keystr, CB_KEYMAXLEN);

			gwinSetText(p->cb_buttons[i], keystr, TRUE);

			free(keystr);
			keystr = NULL;
		} else if (i == CB_OUTPUT)
			gwinSetText(p->cb_buttons[i], p->cb_output, FALSE);
	}

	p->cb_buttons_initd = 1;

	geventListenerInit(&p->cb_listener);
	gwinAttachListener(&p->cb_listener);

	geventRegisterCallback (&p->cb_listener, orchardAppUgfxCallback,
	    &p->cb_listener);

	return;
}

static void
destroy_screen(OrchardAppContext *context)
{
	struct caesar_data *p;
	int i;

	p = context->priv;

	if (!p->cb_buttons_initd)
		return;

        geventRegisterCallback (&p->cb_listener, NULL, NULL);
        geventDetachSource (&p->cb_listener, NULL);

	for (i = 0; i < CB_NBUTTONS; i++)
		gwinDestroy(p->cb_buttons[i]);

	p->cb_buttons_initd = 0;
}

/*
 * Apply the caesar key stream in key.  If there is more input than output,
 * the keystream will start at the begining again.  If enc if true, then
 * the keystream is added.  If it is false, the keystream is subtraced.
 *
 * There MUST be at least one key element, and the key stream is denoted by
 * value >= 26, that is key[0] must be < 26, and key[1] must be valid (exists).
 */
static void
apply_caesar(const char *inp, char *out, const uint8_t *key, int enc)
{
	int inpidx;
	int keyidx;
	int factor;

	if (enc)
		factor = 1;
	else
		factor = -1;
	inpidx = 0;
	keyidx = 0;

	for (; inp[inpidx] != '\x00'; inpidx++) {
		out[inpidx] = toupper(inp[inpidx]);
		if (out[inpidx] < 'A' || out[inpidx] > 'Z')
			continue;

		out[inpidx] += factor * key[keyidx++];
		/*
		 * if we didn't convert to upper, we'd need to make out an
		 * unsigned array for the following comparision to be valid.
		 */
		if (out[inpidx] > 'Z')
			out[inpidx] -= 26;
		else if (out[inpidx] < 'A')
			out[inpidx] += 26;

		if (key[keyidx] >= 26)
			keyidx = 0;
	}
}

static void
caesar_start(OrchardAppContext *context)
{
	struct caesar_data *p;
	userconfig *config;

	p = malloc(sizeof *p);
	*p = (struct caesar_data){};
	context->priv = p;

	config = getConfig();
	if (config->puz_enabled)
		strcpy(p->cb_input, special_message);
	else
		strcpy(p->cb_input, "rapelcggurargohgabgebgguvegrra");
	p->cb_keystream[0] = 13;
	p->cb_keystream[1] = 26;
	p->cb_uicontext.itemlist = &p->cb_itemlist[0];
	p->cb_uicontext.itemlist[0] = NULL;
	p->cb_uicontext.itemlist[1] = p->cb_keyboardinp;

	p->font = gdispOpenFont(FONT_KEYBOARD);

	gdispClear (Black);
	p->o = gdispGetOrientation ();
	gdispSetOrientation (GDISP_DEFAULT_ORIENTATION);

	gwinSetDefaultFont (p->font);

	draw_screen (context);
}

static void
caesar_event(OrchardAppContext *context, const OrchardAppEvent *event)
{
	const OrchardUi * keyboardUi;
	GEvent *pe;
	struct caesar_data *p;
	int i, slen;

	keyboardUi = context->instance->ui;

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
		/* Think that we can always call this w/o issue */
		if (keyboardUi != NULL)
			keyboardUi->event(context, event);

		pe = event->ugfx.pEvent;

		if (pe->type == GEVENT_GWIN_BUTTON) {
			if (p->cb_outputshown && ((GEventGWinButton *)pe)->gwin ==
			    p->cb_outputbut) {
				destroy_output(context);
				draw_screen(context);
			} else {
				for (i = 0; i < CB_NBUTTONS; i++) {
					if (((GEventGWinButton *)pe)->gwin ==
					    p->cb_buttons[i])
						break;
				}
				switch (i) {
				case CB_INPUT:
				case CB_KEY:
				case CB_CAESAR:
					context->instance->ui = getUiByName("keyboard");
					context->instance->uicontext = &p->cb_uicontext;
					p->cb_uicontext.total = CB_MAXLEN - 1;
					p->cb_itemlist[0] = casear_keyboarduitext[i];
					p->cb_inpbutton = i;
					if (i == CB_INPUT)
						strcpy(p->cb_keyboardinp, p->cb_input);
					else if (i == CB_CAESAR) {
						p->cb_uicontext.total = CB_KEYMAXLEN - 1;
						sprintf(p->cb_keyboardinp, "%d",
						    p->cb_keystream[0]);
					} else if (i == CB_KEY)
						keystream_to_str(p->cb_keystream,
						    p->cb_keyboardinp, CB_MAXLEN * 2);

					/* zero out remaining buffer */
					slen = strlen(p->cb_keyboardinp);
					memset(&p->cb_keyboardinp[slen], 0,
					    CB_KEYMAXLEN - slen);

					destroy_screen(context);
					context->instance->ui->start (context);
					break;

				case CB_SWAP:
					{
						char buf[CB_MAXLEN];

						strcpy(buf, p->cb_output);
						strcpy(p->cb_output, p->cb_input);
						strcpy(p->cb_input, buf);

						gwinSetText(p->cb_buttons[CB_INPUT],
						    p->cb_input, FALSE);
						gwinSetText(p->cb_buttons[CB_OUTPUT],
						    p->cb_output, FALSE);
					}
					break;

				case CB_ENCIPHER:
				case CB_DECIPHER:
					memset(p->cb_output, 0, CB_MAXLEN);
					apply_caesar(p->cb_input, p->cb_output,
					    p->cb_keystream, i == CB_ENCIPHER);
					gwinSetText(p->cb_buttons[CB_OUTPUT],
					    p->cb_output, FALSE);
					break;

				case CB_OUTPUT:
					destroy_screen(context);
					draw_output(context);
					break;

				case CB_EXIT:
					orchardAppExit();
					break;

				default:
					/* NOTHING */
					break;
				}
			}
		}
	}

	/* Handle Ui Event */
	if (event->type == uiEvent) {
		if (event->ui.code == uiComplete &&
		    event->ui.flags == uiOK) {
			keyboardUi->exit(context);
			keyboardUi = context->instance->ui = NULL;

			if (p->cb_inpbutton == CB_INPUT)
				strcpy(p->cb_input, p->cb_keyboardinp);
			else if (p->cb_inpbutton == CB_KEY)
				str_to_keystream(p->cb_keyboardinp,
				    p->cb_keystream, CB_MAXLEN);
			else if (p->cb_inpbutton == CB_CAESAR) {
				p->cb_keystream[0] = atoi(p->cb_keyboardinp);
				p->cb_keystream[1] = 26;
			}

			draw_screen(context);
		}
	}

	if (event->type == appEvent && event->app.event == appTerminate) {
		if (keyboardUi != NULL) {
			keyboardUi->exit (context);
			keyboardUi = context->instance->ui = NULL;
		}
	}

}

static void
caesar_exit(OrchardAppContext *context)
{
	struct caesar_data *p;

	p = context->priv;

	destroy_screen(context);

	gdispSetOrientation (p->o);
	gdispCloseFont (p->font);

	geventDetachSource (&p->cb_listener, NULL);
	geventRegisterCallback (&p->cb_listener, NULL, NULL);

	free (p);
	context->priv = NULL;
}

orchard_app("Caesar", "icons/bell.rgb", 0, caesar_init,
             caesar_start, caesar_event, caesar_exit, 9999);
