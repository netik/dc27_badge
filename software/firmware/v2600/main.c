/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for Details.
   
   $Id: main.c,v 1.20 1997/02/23 20:39:12 ahornby Exp ahornby $
******************************************************************************/

/* 
   The main program body.
 */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "display.h"
#include "keyboard.h"
#include "realjoy.h"
#include "files.h"
#include "config.h"
#include "vmachine.h"
#include "profile.h"
#include "options.h"
#include "dbg_mess.h"

#include "sound.h"

/* The mainloop from cpu.c */
extern void mainloop (void);

/* The xdebugger init */
#ifdef XDEBUGGER
extern void init_debugger (void);
#endif

/* Make sure any exit() calls free up shared memory */
void
cleanup (void)
{
  /* Close the raw keyboard */
  close_keyboard ();

  /* Close the PC joystick */
  close_realjoy ();

  /* Close down display */
  tv_off ();

  /* Report on instruction usage */
#ifdef PROFILE
  profile_report ();
#endif
  /* Turn off the sound */
  sound_close ();

}
/* The main entry point */
/* argc: number of command line arguments */
/* argv: the argument text */
int
main (int argc, char **argv)
{

  /* Parse options */
  parse_options (argc, argv);

  dbg_message( DBG_USER, "Virtual 2600 starting...\n");

  /* Initialise the 2600 hardware */
  init_hardware ();

  /* Turn the virtual TV on. */
  tv_on (argc, argv);

  /* Turn on sound */
  sound_init ();

  init_realjoy ();

  /* load cartridge image */
  if (loadCart ())
    {
      fprintf (stderr, "Error loading cartridge image.\n");
      exit (-1);
    }

  /* Cannot be done until file is loaded */
  init_banking();

  /* Set up the cleanup code */
#if HAVE_ATEXIT
  atexit (cleanup);
#else
#if HAVE_ON_EXIT
  on_exit (cleanup);
#endif
#endif

  /* setup debugger */
#ifdef XDEBUGGER
  init_debugger ();
#endif

#ifdef PROFILE
  profile_init ();
#endif

  dbg_message(DBG_USER,"Startup OK\n");

  /* start cpu */
  mainloop ();

  /* Thats it folks */
  exit (0);
}
