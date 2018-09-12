






/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: linux_joy.c,v 1.2 1996/08/26 15:04:20 ahornby Exp $
******************************************************************************/


/* Linux /dev/js* support */
/* All these should be called from the routines in ***_keyb.c */

#include "config.h"
#include "options.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/joystick.h>

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
  struct JS_DATA_TYPE js_data;
  int status;
  if (joyfd[stick] == -1)
    return retval;

  status = read (joyfd[stick], &js_data, JS_RETURN);
  if (status != JS_RETURN)
    {
      perror ("read_realjoy().");
      return retval;
    }
  if (js_data.x > right[stick])
    retval |= RJOYMASK;
  if (js_data.x < left[stick])
    retval |= LJOYMASK;
  if (js_data.y > down[stick])
    retval |= DJOYMASK;
  if (js_data.y < up[stick])
    retval |= UJOYMASK;

  if (js_data.buttons & 1)
    retval |= B1JOYMASK;
  if (js_data.buttons & 2)
    retval |= B2JOYMASK;
  return retval;
}

/* Called once per frame al-la keyboard_update. */
void
update_realjoy (void)
{
  if (base_opts.realjoy == 0)
    return;
  joyres[0] = read_realjoy (0);
  joyres[1] = read_realjoy (1);
}


/* Calibrates the joysticks by there centre position */
void
calibrate_realjoy (int stick)
{
  struct JS_DATA_TYPE js_data;

  if (joyfd[stick] == -1)
    return;

  /* Get the current position, hopefully central. */
  read (joyfd[stick], &js_data, JS_RETURN);

  /* Work out approximate values */
  centre.x = js_data.x;
  centre.y = js_data.y;
  topleft.x = js_data.x - js_data.x;
  topleft.y = js_data.x - js_data.x;
  botright.x = (js_data.x * 2);
  botright.y = (js_data.y * 2);

  /* Set the values used for digital emulation */
  right[stick] = centre.x + (botright.x - centre.x) / 2;
  left[stick] = centre.x - (centre.x - topleft.x) / 2;
  down[stick] = centre.y + (botright.y - centre.y) / 2;
  up[stick] = centre.y - (centre.y - topleft.y) / 2;
}


/* Starts up PC joystick support */
void
init_realjoy (void)
{
  if (base_opts.realjoy == 0)
    return;

  if (Verbose)
    printf ("Initialising PC joystick support\n");

  /* Try to open the devices */
  joyfd[0] = open ("/dev/js0", O_RDONLY);
  joyfd[1] = open ("/dev/js1", O_RDONLY);

  if (joyfd[0] == -1 && joyfd[1] == -1)
    {
      fprintf (stderr, "Sorry no joysticks found, check that the joysticks are plugged in and that\n the joystick module is loaded.\n");
      base_opts.realjoy = 0;
      return;
    }
  else if (joyfd[0] != -1 && joyfd[1] != -1)
    {
      if (Verbose)
	printf ("Joysticks in ports 1 and 2 found\n");
    }
  else if (joyfd[0] != -1 && joyfd[1] == -1)
    {
      if (Verbose)
	printf ("Joystick in port 0 found.\n");
    }
  else if (joyfd[0] == -1 && joyfd[1] != -1)
    {
      if (Verbose)
	printf ("Joystick in port 1 found.\n");
    }

  if (Verbose)
    printf ("Calibrating Joysticks\n");

  calibrate_realjoy (0);
  calibrate_realjoy (1);
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
