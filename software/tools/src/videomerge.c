#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*
 * This program merges a video and audio stream together into a single file
 * for playback on the Nordic NRF52 using our video player. The video stream
 * is a file containing sequential 160x120 resolution frames in 16-bit RGB565
 * pixel format. The audio stream is a file containing a stream of 16-bit
 * audio samples encoded at a rate of 11520Hz, stored 16 bits per sample.
 */

#define VID_FRAMES_PER_SEC		16

#define VID_PIXELS_PER_LINE		160
#define VID_LINES_PER_FRAME		120

#define VID_CHUNK_LINES			16

#define VID_AUDIO_SAMPLES_PER_SCANLINE	6

#define VID_AUDIO_CHANNELS		1

#define VID_AUDIO_SAMPLE_RATE		11520

#define VID_AUDIO_SAMPLES_PER_CHUNK		\
	(VID_AUDIO_SAMPLES_PER_SCANLINE * VID_CHUNK_LINES)

#define VID_AUDIO_BYTES_PER_CHUNK		\
	(VID_AUDIO_SAMPLES_PER_CHUNK * 2)

#define VID_BUFSZ			(VID_PIXELS_PER_LINE * VID_CHUNK_LINES)
#define AUD_BUFSZ			(VID_AUDIO_SAMPLES_PER_CHUNK)

int
main (int argc, char * argv[])
{
	FILE * audio;
	FILE * video;
	FILE * out;
	uint16_t scanline[VID_BUFSZ];
	uint16_t samples[AUD_BUFSZ];

	if (argc > 4)
		exit (1);

	video = fopen (argv[1], "r+");

	if (video == NULL) {
		fprintf (stderr, "[%s]: ", argv[1]);
		perror ("file open failed");
		exit (1);
	}

	audio = fopen (argv[2], "r+");

        if (audio == NULL) {
		fprintf (stderr, "[%s]: ", argv[2]);
		perror ("file open failed");
		exit (1);
	}

	out = fopen (argv[3], "w");

	if (out == NULL) {
		fprintf (stderr, "[%s]: ", argv[3]);
		perror ("file open failed");
		exit (1);
	}

	while (1) {
		memset (samples, 0, sizeof(samples));
		if (fread (scanline, sizeof(scanline), 1, video) == 0) {
			break;
		}
		(void) fread (samples, sizeof(samples), 1, audio);
		fwrite (scanline, sizeof(scanline), 1, out);
		fwrite (samples, sizeof(samples), 1, out);
	}

	fclose (video);
	fclose (audio);
	fclose (out);

	exit(0);
}
