

/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: linux_sound.c,v 1.7 1997/11/22 14:27:47 ahornby Exp ahornby $
******************************************************************************/

/* Linux /dev/dsp  code */

#include "config.h"

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include "types.h"
#include "options.h"
#include "vmachine.h"
#include "tiasound.h"
#include "dbg_mess.h"
#include "qsound.h"

/* Could set this up as an option... */
#define SAMPLE_RATE 22050

/* 0002 = 2 fragments   */
/* 0007 = means each fragment is 2^7 or 128 bytes */
/* See voxware docs (in /usr/src/linux/drivers/sound) for more info */


#define AUDIO_CLOCK 31400

static int sound_change;

#define SINE 0
#define FOURBIT 1
#define FIVEFOURBIT 2
#define FIVEBIT 3
#define NINEBIT 4


/* Sound card file descriptor */
 int sfd=-1;

/* Number of bits to mix */
static int dspbits = 8;

/* Size of a sound block */
static int sndbufsize;

/* The buffer is too large at one second! */
static BYTE buffer[44100], *bufpt;

/* Whether sound is found on start up */
static int have_sound, sound_available;

/* Turn on the sound (hacked from UAE)*/
void
sound_init (void)
{
  int tmp;

  int fragsize,fragpow=0,i;

  /* The sound sample rate */
  int rate=SAMPLE_RATE;
  
  /* What sound formats the sound device takes */
  unsigned long formats;
  
  /* Open the dsp */
  sfd = open ("/dev/dsp", O_WRONLY);
  have_sound = !(sfd < 0);
  if (!have_sound) {    
    return;
  }
  
  /* Get the sound formats */
  ioctl (sfd, SNDCTL_DSP_GETFMTS, &formats);
  
  /* Set the sound of a sound fragment */
  fragsize=rate/30;
  
  for( i=1; i<  255; i++)
    {
      if(pow(2,i) >= fragsize)
	{
	  fragpow=i;
	  break;
	}
    }
  tmp = 0x00020000 + fragpow;
  ioctl (sfd, SNDCTL_DSP_SETFRAGMENT, &tmp);
  
  /* Get the preferred value */
  ioctl (sfd, SNDCTL_DSP_GETBLKSIZE, &sndbufsize);

  
  /* Set for 8bit sound */
  dspbits = 8;
  ioctl(sfd, SNDCTL_DSP_SAMPLESIZE, &dspbits);
  ioctl(sfd, SOUND_PCM_READ_BITS, &dspbits);

  /* Do not set stereo 
     tmp = 0;
     ioctl(sfd, SNDCTL_DSP_STEREO, &tmp);
     */

  /* Aim for low quality */
  ioctl(sfd, SNDCTL_DSP_SPEED, &rate);
  ioctl(sfd, SOUND_PCM_READ_RATE, &rate);
  

  if (!(formats & AFMT_U8))
    return;

  if(base_opts.sound)
    {
      sound_available = 1;
      
      dbg_message(DBG_USER,"Sound driver found and configured for %d bits, %d Hz, buffer is %d bytes\n", dspbits, rate, sndbufsize);

      bufpt = buffer;
      qsound_init(AUDIO_CLOCK, rate, sndbufsize);
    }
}


long
sound_get_freespace (void)
{
  audio_buf_info info;
  int i;

  i=ioctl(sfd,SNDCTL_DSP_GETOSPACE,&info);
  if (i<0) 
    { 
      perror("SNDCTL_DSP_GETOSPACE");
      return -1; 
    }

  return (long)(info.bytes);
}

void
sound_update(void)
{
  if(sound_available)
    {
      int i;
      /* Tia_process(bufpt, sndbufsize);*/
      
      /* Get the number of bytes to write */
      i=sound_get_freespace();
      if(i>100)
	{
	  qsound_process(bufpt, i); 
	  bufpt = buffer;
	  write(sfd, buffer, i);
	}
    }
}

/* Turn off the sound */
void
sound_close (void)
{
  if(sfd!=-1)
    close(sfd);
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










