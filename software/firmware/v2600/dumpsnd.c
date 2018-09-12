



#include <stdio.h>
#include <math.h>
#ifndef SYSV
#include <audio/Aos.h>		/* for string and other os stuff */
#endif
#include <audio/Afuncs.h>	/* for bcopy et. al. */
#include <audio/audiolib.h>
#include <audio/soundlib.h>

/* Produce a DESIRED_RATEhz wave in an SAMPLE_RATE sample */
/* But only produce 1/50th of a second of it! */


#define SAMPLE_RATE 11025
int DESIRED_RATE = 1000;

WaveInfo sound =
{NULL, "", 1, 16, SAMPLE_RATE, sizeof (WaveInfo), 1, 0, 0, 0, 0, 0};

void
main (int argc, char **argv)
{
  int i;
  signed short value[65536];
  double sval;
  double divisor = SAMPLE_RATE / DESIRED_RATE;
  int NUM_SAMPLES = SAMPLE_RATE / 25;

  /* Aim for a 1 sec sample */
  /* A 1kHz sine wave will have 1k peaks in the 11k so 11k/1k=11.05 
     samples per wave form. */
  for (i = 0; i < NUM_SAMPLES; i++)
    {
      /* 0=0 PI/2=1 PI=0 PI3/2=-1 2PI=0 */
      sval = fmod ((double) i, divisor);
      /* Rescale to the range 0 to 2PI */
      sval = (sval * 2 * PI) / divisor;
      value[i] = (double) sin (sval) * 32767;
    }

  WaveOpenFileForWriting ("./dump.wav", &sound);
  WaveWriteFile ((char *) value, sizeof (short) * NUM_SAMPLES, &sound);
  WaveCloseFile (&sound);
}
