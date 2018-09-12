#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* this program outputs a simple 8 byte header for rgb565 files */
/* prepend this to a raw rgb565 file format to load the image on our display */

#define GDISP_COLORSYSTEM_RGB           0x2000
#define GDISP_PIXELFORMAT_RGB565        (GDISP_COLORSYSTEM_RGB|0x0565)

typedef struct gdisp_image {
	uint8_t			gdi_id1;
	uint8_t			gdi_id2;
	uint8_t			gdi_width_hi;
	uint8_t			gdi_width_lo;
	uint8_t			gdi_height_hi;
	uint8_t			gdi_height_lo;
	uint8_t			gdi_fmt_hi;
	uint8_t			gdi_fmt_lo;
} GDISP_IMAGE;

int
main (int argc, char * argv[])
{
	FILE *			fp;
	GDISP_IMAGE		img;
	uint16_t		width;
	uint16_t		height;

	if (argc < 4) {
	  fprintf (stderr, "\nUsage: %s width height output_filename\n\n", argv[0]);
	  exit (1);
	}

	width = atoi(argv[1]);
	height = atoi(argv[2]);

	img.gdi_id1 = 'N';
	img.gdi_id2 = 'I';
	img.gdi_width_hi = width >> 8;
	img.gdi_width_lo = width & 0xFF;
	img.gdi_height_hi = height >> 8;
	img.gdi_height_lo = height & 0xFF;
	img.gdi_fmt_hi = (GDISP_PIXELFORMAT_RGB565) >> 8;
	img.gdi_fmt_lo = (GDISP_PIXELFORMAT_RGB565) & 0xFF ;

	fp = fopen (argv[3], "w");

	if (fp == NULL) {
		perror ("Output file open failed.");
		exit (1);
	}

	fwrite ((void *)&img, sizeof(img), 1, fp);

	fclose (fp);

	exit(0);
}

