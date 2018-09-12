



/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: nasnew.c,v 1.1 1996/11/24 16:55:40 ahornby Exp $
******************************************************************************/

/* Network Audio System code */

#include "config.h"
#include <stdio.h>
#ifndef SYSV
#include <audio/Aos.h>		/* for string and other os stuff */
#endif
#include <audio/Afuncs.h>	/* for bcopy et. al. */
#include <audio/audiolib.h>
#include <audio/soundlib.h>
#include "options.h"


/* Could set this up as an option... */
#define SAMPLE_RATE 11025

#define AUDIO_CLOCK 30000

/* Flag to set if sound changes */
static int sound_change = 1;

/* NAS volume, frequency, and flow numbers */
static int x26_vol[2], x26_freq[2];

/* The flow ID's */
static AuFlowID x26_flow[2];

/* The NAS device */
static AuDeviceID *x26_device;

/* Set to null to get the default audio server name */
static AuServer *aud;

/* The elements to send to the server. */
/* 0 = volume, 1= device. 2 this bucket 3=next bucket */
AuElement elements[4][2];

/* The Atari 2600 waveform, not the NAS one! */
static int wave_form[2];

static int buck_num[2];
static int buck_freq[2];


#define SINE 0
#define FOURBIT 1
#define FIVEFOURBIT 2
#define FIVEBIT 3
#define NINEBIT 4

/* The buckets that store the various waveforms */
static AuBucketID bucket[5];

/* Connect to the audio server */
static void
connect (void)
{
  char *audioServer = NULL;

  /* First find the audio server */
  aud = AuOpenServer (audioServer, 0, NULL, 0, NULL, NULL);
  if (aud == NULL)
    {
      fprintf (stderr, "x2600: Error opening audio server:\n");
      fprintf (stderr, "Make sure a server is running\n");
      exit (1);
    }
}

/* Check that the waveforms we need are there */
static void
check_waves (void)
{
  int i, max, waveval;
  char *wavedesc;
  max = AuServerNumWaveForms (aud);

  if (Verbose)
    printf ("Checking waveforms available.\n");
  for (i = 0; i < max; i++)
    {
      waveval = AuServerWaveForm (aud, i);
      wavedesc = (char *) AuWaveFormToString (waveval);
      if (Verbose)
	printf ("Waveform %d: %s\n", i, wavedesc);
    }
}

void
start_flow (void)
{
  int i;
  AuStatus status;
  char etext[80];
  /* Initial frequency and volume levels */
  int volume = 0;
  int freq = 0;

  /* Pick the correct device */
  for (x26_device = NULL, i = 0; i < AuServerNumDevices (aud); i++)
    {
      if ((AuDeviceKind (AuServerDevice (aud, i)) ==
	   AuComponentKindPhysicalOutput) &&
	  AuDeviceNumTracks (AuServerDevice (aud, i)) == 1)
	{
	  x26_device = (AuDeviceID *)
	    AuDeviceIdentifier (AuServerDevice (aud, i));
	  break;
	}
    }

  /* Create the two flows */
  /* this->next->volume_control->device */
  for (i = 0; i < 2; i++)
    {
      x26_flow[i] = AuCreateFlow (aud, NULL);
      x26_freq[i] = 440;
      x26_vol[i] = 100;
      buck_freq[i] = 440;
      buck_num[i] = SINE;

      /* Initialise bucket elements */
      AuMakeElementImportBucket (&elements[0][i], 11025,
				 bucket[SINE],
				 AuUnlimitedSamples, 0, 0, NULL);

      /* Initialise bucket elements 
         AuMakeElementImportBucket(&elements[i][1], 11025,
         bucket[SINE], 
         AuUnlimitedSamples, 0, 0, NULL); */

      /* Initialise volume control */
      AuMakeElementMultiplyConstant (&elements[1][i], 1,
				     AuFixedPointFromFraction (volume, 100));

      /* Initialise device, the int cast should * according to the docs! */
      AuMakeElementExportDevice (&elements[2][i], 2, (int) x26_device,
				 SAMPLE_RATE, AuUnlimitedSamples, 0, NULL);

      /* Set the volume. freq etc into the flow. */
      AuSetElements (aud, x26_flow[i], AuTrue, 3, &elements[0][i], &status);
      if (status != AuSuccess)
	{
	  AuGetErrorText (aud, status, etext, 80);
	  fprintf (stderr, "Bad set elements on flow %d:\n %s\n", i, etext);
	  exit (-1);
	}

      if (Verbose)
	printf ("Starting NAS flow %d\n", i);
      /* Start the flow off */
      AuStartFlow (aud, x26_flow[i], &status);
      if (status != AuSuccess)
	{
	  fprintf (stderr, "Bad start on flow %d\n", i);
	  exit (-1);
	}
    }
  AuFlush (aud);
}


void
load_buckets (void)
{
  AuStatus status;
  AuUint32 access = AuAccessAllMasks;

  bucket[SINE] = AuSoundCreateBucketFromFile (aud, "./sounds/sine1000.wav",
					      access, NULL, &status);
  if (bucket[SINE] == 0)
    {
      fprintf (stderr, "Bad bucket load");
      exit (-1);
    }
}

/* Turn on the sound */
void
sound_init (void)
{
  int i;
  if (!base_opts.sound)
    return;
  for (i = 0; i < 2; i++)
    wave_form[i] = 0;
  connect ();
  load_buckets ();
  start_flow ();
}


/* Turn off the sound */
void
sound_close (void)
{
  int i;
  /* Quick sanity check */
  if (aud != NULL)
    {
      for (i = 0; i < 2; i++)
	AuDestroyFlow (aud, x26_flow[i], NULL);
      AuCloseServer (aud);
    }
}


/* Play the sound */
/* channel: 0 or 1 */
static void
play_sound (int channel)
{
  AuElementParameters parms;

  int value = x26_freq[channel];
  /* Work out frequency to play at from waveform and freq */
  switch (wave_form[channel])
    {
    case 0:
      /*set to 1 */
      value = 0;
      AuPauseFlow (aud, x26_flow[channel], NULL);
      break;
    case 1:
      /* 4 bit poly */
      value = 0;
      break;
    case 2:
      /* div 15 -> 4-bit poly */
      value /= 15;
      break;
    case 3:
      /* 5 bit poly -> 4 bit poly */
      break;
    case 4:
    case 5:
      /* div 2: pure tone */
      buck_num[channel] = SINE;
      value /= 2;
      break;
    case 6:
      /* div 31: pure tone */
      buck_num[channel] = SINE;
      value /= 31;
      break;
    case 7:
      /* 5 bit poly -> div 2 */
      printf ("Five bit poly div 2 played\n");
      value /= 2;
      break;
    case 8:
      /* 9 bit poly (white noise) */
      value = 0;
      break;
    case 9:
      /* 5 bit poly */
      printf ("Five bit poly played on %d, frequency=%d\n", channel, value);
      value = 0;
      break;
    case 10:
      /* div 31: pure tone */
      buck_num[channel] = SINE;
      value /= 31;
      break;
    case 11:
      /* Set last four bits to one! */
      value = 0;
      break;
    case 12:
    case 13:
      /* div 6: pure tone */
      buck_num[channel] = SINE;
      value /= 6;
      break;
    case 14:
      /* div 93: pure tone */
      buck_num[channel] = SINE;
      value /= 93;
      break;
    case 15:
      /* 5 bit poly div 6 */
      printf ("Five bit poly div 6 played\n");
      value /= 6;
      break;
    default:
      fprintf (stderr, "Error in NAS sound %d\n", wave_form[channel]);
      exit (-1);
    }

  buck_freq[channel] = value;
  if (value > 29000)
    printf ("High freq!");
  sound_change = 1;
}

/* Set the waveform from the audc0/1 registers */
/* channel: 0 or 1 */
/* value: 0 to 16 */
void
sound_waveform (int channel, BYTE value)
{
  if (!base_opts.sound)
    return;
  wave_form[channel] = value;
  play_sound (channel);
}

/* Set the volume from the audv0/1 registers */
/* channel: 0 or 1 */
/* vol: 0(off) to 15(loudest) */
void
sound_volume (int channel, BYTE vol)
{
  static AuElementParameters parms;
  if (!base_opts.sound)
    return;
  printf ("Set volume chan %d, %d\n", channel, (int) vol);
  /* Check for duplicates */
  if (vol != x26_vol[channel])
    {
      parms.flow = x26_flow[channel];
      parms.element_num = 1;
      parms.num_parameters = AuParmsMultiplyConstant;
      parms.parameters[AuParmsMultiplyConstantConstant] =
	AuFixedPointFromFraction (vol, 15);
      AuSetElementParameters (aud, 1, &parms, NULL);
      sound_change = 1;
    }
}

/* Set the sound frequency from the audf0/1 registers */
/* channel: 0 or 1 */
/* freq: 0 to 31, ie division from 1 to 32 */
void
sound_freq (int channel, BYTE freq)
{
  if (!base_opts.sound)
    return;
  x26_freq[channel] = AUDIO_CLOCK / (freq + 1);
  play_sound (channel);
}


static int s_this[2] =
{2, 3}, s_next[2] =
{4, 5};

/* Prepare the next bucket for loading onto */
/* channel: flow to use */
static void
sound_prep (int channel)
{
  AuBucketID bid;
  AuElementAction action;
  int freq;

  /* Get the bucket for the current waveform */
  bid = bucket[buck_num[channel]];

  /* Set up an action so that when this is started the old one is stopped */
  AuMakeChangeStateAction (&action, AuStateStart, AuStateAny,
			   AuReasonAny, x26_flow[channel], 2, AuStateStop);

  /* Set the frequency of to play the chosen bucket. */
  freq = buck_freq[channel] * SAMPLE_RATE / 1000;

  /* Initialise bucket elements */
  AuMakeElementImportBucket (&elements[channel][s_next[channel]], (short) freq,
			     bid,
			     AuUnlimitedSamples, 0, 1, &action);

  /* Set the volume. freq etc into the flow. */
  AuSetElements (aud, x26_flow[channel], AuTrue, 6, elements[channel], NULL);
}

static void
sound_play (int channel)
{
  AuElementState state;

  /* Start this element */
  AuMakeElementState (&state,
		      x26_flow[channel], s_this[channel], AuStateStart);

  /* Which automatically stops the old channel */
  AuSetElementStates (aud, 1, &state, NULL);
}

/* Called every screen refresh */
/* Basic philosophy: Whilst playing from element 1 load element 2
   and so on. */
/* Each element then has an action to start the next one when finished */
/* So, hopefully, no gaps! NOTE make samples ~4 frames i.e. 4/50ths  */
void
sound_flush (void)
{
  int i, temp;

  sound_play (0);
  sound_play (1);
  sound_prep (0);
  sound_prep (1);

  /* Swap Values */
  for (i = 0; i < 2; i++)
    {
      temp = s_this[i];
      s_this[i] = s_next[i];
      s_next[i] = s_this[i];
    }
}
