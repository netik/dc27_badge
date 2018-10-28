#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*
 * This program serves one purpose, which is to skip every 625th sample
 * in a raw audio file. The reason we have to do this is that when using
 * an audio sample rate of 15625Hz, we can't get a whole number of samples
 * per video scanline. Currently we use 20 samples per scanline. That
 * actually works out to 10 stereo samples. That gives us the following:
 *
 * 13 frames/sec * 120 lines per frame == 1560 lines/sec
 * 1560 lines/sec * 20 samples per line == 31200 samples/sec
 * 15625 samples/sec * 2 channels == 31250 samples/sec
 *
 * This means that we can only get 31200 samples per second. If we don't
 * compensate for this, the audio will gradually drift out of sync with
 * the video. So we need to drop a sample every once in a while.
 *
 * 31250 / 625 happens to be 50. So if we drop every 625th sample, this
 * will bring us down to 31200 samples/second.
 *
 * This is all Nordic's fault for not providing a more configurable
 * I2S peripheral block.
 */
 
#define AUD_BUFSZ			(624 * 2)

int
main (int argc, char * argv[])
{
	FILE * audio;
	FILE * out;
	uint16_t samples[AUD_BUFSZ];

	if (argc > 3)
		exit (1);

	audio = fopen (argv[1], "r+");

        if (audio == NULL) {
		fprintf (stderr, "[%s]: ", argv[2]);
		perror ("file open failed");
		exit (1);
	}

	out = fopen (argv[2], "w");

	if (out == NULL) {
		fprintf (stderr, "[%s]: ", argv[3]);
		perror ("file open failed");
		exit (1);
	}

	while (1) {
		if (fread (samples, sizeof(samples), 1, audio) == 0)
			break;
		fwrite (samples, sizeof(samples), 1, out);
		fseek (audio, 4, SEEK_CUR);
	}

	fclose (audio);
	fclose (out);

	exit(0);
}
