/* Functions to do quicker sound processing */
/* Copyright Alex Hornby 1997 */

#include "config.h"
#include <stdio.h>

#include "types.h"
#include "dbg_mess.h"

#define uint8  unsigned char  
#define AUDIO_CLOCK 31400.0

static int BUFFER_CLOCK=0;
static unsigned char *qbuffers[16]

void 
qsound_init ( int sample_freq, int playback_freq, int bufsize)
{
  /* Keep things simple */
  BUFFER_CLOCK=playback_freq;

  /* Fill the qsound buffers with 2600 sounds at half the Audio Clock Rate */
  dbg_message(DBG_NORMAL, "Setting up sound buffers\n");

  /* Sets the freqency of the channel. Keep this constant */
  Update_tia_sound(0x17, BUFFER_CLOCK);
  
  /* Sets the volume of the channel to maximum.  Keep this constant! */  
  Update_tia_sound(0x19, 0x0f);

  /* Turn off the other channel! */
  Update_tia_sound(0x19 + 1 , 0x00);

  /* Loop through each of the waveforms */
  for( i=0, i<16; i++)
    {
      /* Allocate some memory for the buffer if needed. */
      if( qbuffers[i]=0 )
	qbuffers[i]=malloc(bufsize);
	  
      Update_tia_sound(0x15, i);     
      Tia_process( qbuffers[i], bufsize); 
    }
}




/* Builds the current sound buffer in memory */
void 
qsound_process ( unsigned char *buffer,  UINT16 n)
{
  uint8 chan, outvol[2];
  uint8 *current_buffers[2];
  int offset[2];
  double qbuff_max[2], qbuff_count[2];
    

  /* Link to the TIA registers */
  extern uint8 AUDC[2];    /* AUDCx (15, 16) */
  extern uint8 AUDF[2];    /* AUDFx (17, 18) */
  extern uint8 AUDV[2];    /* AUDVx (19, 1A) */
  
  /* loop through the channels */
  for (chan = 0; chan <= 1; chan++)
    {
      /* Sets the buffer pointer */
      current_buffers[chan]=qbuffers[AUDC[chan]];      
      offset[chan]=0;
      /* Set 2600 clocks per buffer clock */
      qbuff_max[chan]= (AUDIO_CLOCK/ (AUDF[chan] +1))  / BUFFER_CLOCK
    }
  
  /* loop until the buffer is filled */
  while (n)
    {
      /* loop through the channels */
      for (chan = 0; chan <= 1; chan++)
	{
	  outvol[chan]=current_buffers[chan]+offset[chan];
	  
	  /* Increase the offset if the 2600 frequency is > 
	     than the qsound frequency */
	  qbuff_count[chan] --;
	  if(qbuff_count[chan] < 1)
	    {
	      qbuff_count[chan] += qbuff_max[chan];
	      offset[chan]++;
	    }	    
	}
      /* We Now have two values at the qsound frequency */
      
      /* calculate the latest output value and place in buffer */
      *(buffer++) = outvol[0] + outvol[1];
      
      /* and indicate one less byte to process */
      n--;
    }
}

