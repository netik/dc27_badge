#include "ch.h"
#include "hal.h"
#include "osal.h"

#include "badge.h"

#include <stdlib.h>
#include <string.h>

#include "ff.h"
#include "ffconf.h"
#include "diskio.h"

#include "types.h"
#include "display.h"
#include "keyboard.h"
#include "realjoy.h"
#include "config.h"
#include "vmachine.h"
#include "collision.h"
#include "options.h"

/* The mainloop from cpu.c */
extern void mainloop (void);
extern void create_window (void);

static THD_WORKING_AREA(waAtariThread, 256);

static THD_FUNCTION(atariThread, arg)
{
	FIL f;
	UINT br;

	(void)arg;
    
	chRegSetThreadName ("Atari");

	theCart = (BYTE *)0x20000100;
	colvect = (BYTE *)(0x20000100 + 8192);
	base_opts.rr = 1;
	base_opts.magstep = 1;
	base_opts.bank = 1;

	/* Initialise the 2600 hardware */
	init_hardware ();

	/* Turn the virtual TV on. */
	create_window ();
	tv_on (0, NULL);

	f_open (&f, "MSPACMAN.BIN", FA_READ);
	f_read (&f, theCart, 8192, &br);
	f_close (&f);
	rom_size = br;

	init_banking ();

	mainloop ();

	return;
}

void
atariRun (void)
{
	chThdCreateStatic (waAtariThread, sizeof(waAtariThread),
	    129, atariThread, NULL);

	return;
}

