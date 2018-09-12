

/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: nas_sound.c,v 1.6 1996/12/03 22:35:05 ahornby Exp $
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

/* The Atari 2600 waveform, not the NAS one! */
static int wave_form[2];

#define SINE 0
#define FOURBIT 1
#define FIVEFOURBIT 2
#define FIVEBIT 3
#define NINEBIT 4

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
      fprintf (stderr, "v2600: Error opening audio server:\n");
      fprintf (stderr, "Make sure a server is running\n");
      base_opts.sound=0;
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

  /* Initial frequency and volume levels */
  int volume = 0;
  int freq = 0;
  AuElement elements[3];
  for (x26_device = NULL, i = 0; i < AuServerNumDevices (aud); i++)
    {
      if ((AuDeviceKind (AuServerDevice (aud, i)) ==
	   AuComponentKindPhysicalOutput) &&
	  AuDeviceNumTracks (AuServerDevice (aud, i)) == 1)
	{
	  x26_device = (AuDeviceID *) AuDeviceIdentifier (AuServerDevice (aud, i));
	  break;
	}
    }

  /* Initialise waveform */
  AuMakeElementImportWaveForm (&elements[0], SAMPLE_RATE,
			       AuWaveFormSine,
			       AuUnlimitedSamples, freq, 0, NULL);

  /* Initialise volume control */
  AuMakeElementMultiplyConstant (&elements[1], 0,
				 AuFixedPointFromFraction (volume, 100));

  /* Initialise device, the int cast should be a * according to the docs! */
  AuMakeElementExportDevice (&elements[2], 1, (int) x26_device, SAMPLE_RATE,
			     AuUnlimitedSamples, 0, NULL);

  /* Create the two flows */
  for (i = 0; i < 2; i++)
    {
      x26_freq[i] = 440;
      x26_vol[i] = 100;

      x26_flow[i] = AuCreateFlow (aud, NULL);

      /* Set the volume. freq etc into the flow. */
      AuSetElements (aud, x26_flow[i], AuTrue, 3, elements, NULL);

      if (Verbose)
	printf ("Starting flow %d\n", i);
      /* Start the flow off */
      AuStartFlow (aud, x26_flow[i], NULL);
      AuFlush (aud);
    }


/* Stop the flow 
   AuStopFlow(aud, x26_flow, NULL); */
}


void
load_buckets (void)
{
  AuStatus status;
  bucket[SINE] = AuSoundCreateBucketFromFile (aud, "./samples/sine1000.wav",
					      0, NULL, &status);
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
  if (!base_opts.sound)
    return;
  check_waves ();
  start_flow ();
  load_buckets ();
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
      value /= 2;
      break;
    case 6:
      /* div 31: pure tone */
      value /= 31;
      break;
    case 7:
      /* 5 bit poly -> div 2 */
      /*      printf ("Five bit poly div 2 played\n");*/
      value /= 2;
      break;
    case 8:
      /* 9 bit poly (white noise) */
      value = 0;
      break;
    case 9:
      /* 5 bit poly */
      /* printf ("Five bit poly played on %d, frequency=%d\n", channel, value);*/
      value = 0;
      break;
    case 10:
      /* div 31: pure tone */
      value /= 31;
      break;
    case 11:
      /* Set last four bits to one! */
      value = 0;
      break;
    case 12:
    case 13:
      /* div 6: pure tone */
      value /= 6;
      break;
    case 14:
      /* div 93: pure tone */
      value /= 93;
      break;
    case 15:
      /* 5 bit poly div 6 */
      /* printf ("Five bit poly div 6 played\n"); */
      value /= 6;
      break;
    default:
      fprintf (stderr, "Error in NAS sound %d\n", wave_form[channel]);
      exit (-1);
    }

  /* Always play pure sine for now! */
  parms.flow = x26_flow[channel];
  parms.element_num = 0;
  parms.num_parameters = AuParmsImportWaveForm;

  if (value > 29000)
    printf ("High freq!");
  parms.parameters[AuParmsImportWaveFormFrequency] = value;
  parms.parameters[AuParmsImportWaveFormNumSamples] = AuUnlimitedSamples;
  AuSetElementParameters (aud, 1, &parms, NULL);
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
  /*  printf ("Set volume chan %d, %d\n", channel, (int) vol); */
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

void
sound_flush (void)
{
  if (!base_opts.sound)
    return;
  if (sound_change)
    AuFlush (aud);
  sound_change = 0;
}


void
sound_update (void)
{
  if (!base_opts.sound)
    return;
  if (sound_change)
    AuFlush (aud);
  sound_change = 0;
}
