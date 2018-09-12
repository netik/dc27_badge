/* Functions to do quicker sound processing */
/* Copyright Alex Hornby 1997 */

#include "config.h"
#include "unistd.h"
#include <stdio.h>
#include <stdlib.h>
#include "tiasound.h"
#include <assert.h>

#include "types.h"
#include "dbg_mess.h"

#define uint8  unsigned char  
#define AUDIO_CLOCK 31400.0
#define BUFFER_RATIO 2
#define BUFFER_SAMPLE AUDIO_CLOCK/2

extern int sfd;
static int BUFFER_CLOCK=0;
static int qplay_back=0;
static unsigned char *qbuffers[16][32];

void 
qsound_init ( int sample_freq, int playback_freq, int bufsize)
{
  /* Counter */
  int i,j;

  Tia_sound_init(AUDIO_CLOCK, playback_freq); 

  qplay_back=BUFFER_SAMPLE;
  /* Keep things simple */
  BUFFER_CLOCK=AUDIO_CLOCK;

  /* Fill the qsound buffers with 2600 sounds at half the Audio Clock Rate */
  dbg_message(DBG_NORMAL, "Setting up sound buffers\n");
  
  /* Sets the volume of the channel to maximum.  Keep this constant! */  
  Update_tia_sound(0x19, 0x0f);

  /* Turn off the other channel! */
  Update_tia_sound(0x19 + 1 , 0x00);

  /* Loop through each of the waveforms */
  for( i=0; i<16; i++)
    {

      /* Loop through each to the 32 notes*/
      for ( j=0; j<32; j++)
	{
	  /* Sets the volume of the channel to maximum.  Keep this constant! */  	  Update_tia_sound(0x19, 0x0f);
	  
	  /* Set the waveform */
	  Update_tia_sound(0x15, i);

	  /* Sets the freqency of the channel */
	  Update_tia_sound(0x17, j);
	  /* Allocate some memory for the buffer if needed. */
	  if( qbuffers[i][j]==0 )
	    qbuffers[i][j]=malloc(bufsize);
	  
	  /* Store it */
	  Tia_process( qbuffers[i][j], bufsize);

	  /* Turn the channel off again */
	  Update_tia_sound(0x15 , 0x00);
	  /* Sets the volume of the channel to minimum.  Keep this constant! */
	  Update_tia_sound(0x19, 0x00);
	}
    }
  Update_tia_sound(0x19, 0x0F);
  Update_tia_sound(0x19+1, 0x0F);
}

/* Builds the current sound buffer in memory */
void 
qsound_process ( unsigned char *buffer,  UINT16 n)
{
  int chan;
  uint8 outvol[2];
  uint8 *current_buffers[2];
  int offset ;
  int qbuff_vol[2];    
  
  /* Link to the TIA registers */
  extern uint8 AUDC[2];    /* AUDCx (15, 16) */
  extern uint8 AUDF[2];    /* AUDFx (17, 18) */
  extern uint8 AUDV[2];    /* AUDVx (19, 1A) */
  
  int naudv[2];
  naudv[0]=(AUDV[0]>>3);
  naudv[1]=(AUDV[1]>>3);


  /* loop through the channels */
  for (chan = 0; chan <=1; chan++)
    {
      /* Sets the buffer pointer to the correct sample*/
      current_buffers[chan]=qbuffers[AUDC[chan]][AUDF[chan]];     

      qbuff_vol[chan]=AUDV[chan];
    }

  offset=0;
  /* loop until the buffer is filled */
  while (n>0)
    {
      /* Set the output volume */
      outvol[0]=(*(current_buffers[0]++) * naudv[0]) >> 4;
      outvol[1]=(*(current_buffers[1]++) * naudv[1]) >> 4;
      /* We Now have two values at the qsound frequency */          
      /* calculate the latest output value and place in buffer */
      *(buffer++) = outvol[0] + outvol[1];
      
      /* and indicate one less byte to process */
      n--;
    }
}



