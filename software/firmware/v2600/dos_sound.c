/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: dos_sound.c,v 1.2 1997/04/04 22:01:48 ahornby Exp $
******************************************************************************/

/* DOS allegro sound code */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <allegro.h>

#include "types.h"
#include "options.h"
#include "vmachine.h"
#include "tiasound.h"

/* Could set this up as an option... */
#define SAMPLE_RATE 15700

#define AUDIO_CLOCK 31400

static int sound_change;

#define SINE 0
#define FOURBIT 1
#define FIVEFOURBIT 2
#define FIVEBIT 3
#define NINEBIT 4

/* Number of bits to mix */
static int dspbits = 8;

/* Size of a sound block from allegro sb.c*/
static int sndbufsize=2048;

/* The buffer is too large at one second! */
/* static BYTE buffer[44100], *bufpt; */
static BYTE *bufpt;

/* Allegro sample structure */
SAMPLE spl;

/* Whether sound is found on start up */
static int have_sound, sound_available;

/* Turn on the sound */
void
sound_init (void)
{
  int tmp,i;

  if(base_opts.sound)
     { 
     /* Install timer */
     install_timer();

     /* Open the allegro sound driver */
     i = install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL);
     if (i != 0) {
        printf("  ERROR: could not initialize sound card\n   %s\n",allegro_error);
        printf("  Sound system disabled\n");
        base_opts.sound=0;
     }
     else {
        set_volume(250,250);  
        /* Set for 8bit sound */
        spl.bits=8;
        spl.freq=SAMPLE_RATE;
        spl.len=sndbufsize;
        spl.data=malloc(spl.len);

        sound_available = 1;
	if (Verbose) {
  	    printf("Sound driver found and configured for %d bits at %d Hz, buffer is %d bytes\n", spl.bits, SAMPLE_RATE, sndbufsize);
            printf("  Sound driver %s detected\n",digi_driver->name);
        }
	  
	bufpt = spl.data;
	  
        Tia_sound_init(AUDIO_CLOCK, SAMPLE_RATE); 
	play_sample(&spl, 255, 128, 1000, 1);
	}
    }
}


void
sound_update(void)
{
  if (sound_available)
	{
	  Tia_process(spl.data, sndbufsize);
	}
}

/* Turn off the sound */
void
sound_close (void)
{
   if (base_opts.sound) {
      stop_sample(&spl);
      free(spl.data);  
      remove_sound();
      remove_timer();
   }
}

void 
sound_freq(int channel, BYTE freq)
{

  if(sound_available)
	{
	  Update_tia_sound(0x17 + channel, freq);
	}
}

void
sound_volume(int channel, BYTE vol)
{
  if(sound_available)
	{
	  Update_tia_sound(0x19 + channel, vol);
	}
}

void
sound_waveform(int channel, BYTE value)
{
  if(sound_available)
	{
	  Update_tia_sound(0x15 + channel, value);
	}
}

