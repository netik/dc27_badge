/*****************************************************************************

   This file is part of Virtual 2600, the Atari 2600 Emulator
   ==========================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: dos_joy.c,v 1.1 1996/08/28 14:32:34 ahornby Exp $
******************************************************************************/


/* DOS Allegro joystick support */
/* All these should be called from the routines in ***_keyb.c */

#include "config.h"
#include "options.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <allegro.h>

static int joyfd[2] =
{-1, -1};
static int joyres[2] =
{0, 0};
static int left[2], right[2], up[2], down[2];

struct Point
  {
    int x, y;
  };

#define LJOYMASK 0x01
#define RJOYMASK 0x02
#define UJOYMASK 0x04
#define DJOYMASK 0x08
#define B1JOYMASK 0x10
#define B2JOYMASK 0x20

struct Point centre, botright, topleft;

int
get_realjoy (int stick)
{
  return joyres[stick];
}

/* Fetches the current joystick value into joymask */
/* Not called externally */
static int
read_realjoy (int stick)
{
  int retval = 0;

  if (joy_right)
    retval |= RJOYMASK;
  if (joy_left)
    retval |= LJOYMASK;
  if (joy_down)
    retval |= DJOYMASK;
  if (joy_up)
    retval |= UJOYMASK;

  if (joy_b1)
    retval |= B1JOYMASK;
  if (joy_b2)
    retval |= B2JOYMASK;
  return retval;
}

/* Called once per frame al-la keyboard_update. */
void
update_realjoy (void)
{
  poll_joystick();
  if (base_opts.realjoy == 0)
    return;
  joyres[0] = read_realjoy (0);
  joyres[1] = read_realjoy (1);
}

/* Starts up PC joystick support */
void
init_realjoy (void)
{
  if (base_opts.realjoy == 0)
    return;

  if (Verbose)
    printf ("Initialising PC joystick support\n");

  if(initialise_joystick())
  {
      fprintf (stderr, "Sorry no joysticks found, check that the joysticks are plugged in and that\n the joystick module is loaded.\n");
      base_opts.realjoy = 0;
      return;
  }
}

void
close_realjoy (void)
{
  int i;
  for (i = 0; i < 2; i++)
    {
      if (joyfd[i] != -1)
	close (joyfd[i]);
      joyfd[i] = -1;
    }
}
