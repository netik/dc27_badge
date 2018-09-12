#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "c26def.h"

/* Supports a subset of version 2 */

int
main (int argc, char **argv)
{
  FILE *infile, *outfile;
  char outname[80];
  char inbuf[16384];
  char *writer = "bin2c26 Version 1.0";
  int i, namelen;
  struct c26_tag tag;

  for (i = 1; i < argc; i++)
    {
      infile = fopen (argv[i], "rb");
      if (!infile)
	{
	  fprintf (stderr, "Can't find %s\n", argv[i]);
	  exit - 1;
	}

      /* Build the output name */
      namelen = strlen (argv[i]);
      strncpy (outname, argv[i], namelen - 3);
      outname[namelen] = 0;
      strncpy (&outname[namelen - 3], "c26", 3);

      /* Open output file */
      outfile = fopen (outname, "wb");
      if (!outfile)
	{
	  fprintf (stderr, "Can't create %s\n", outname);
	  exit - 1;
	}

      tag.type = C26MAGIC;
      fwrite (&(tag.type), sizeof (tag.type), 1, outfile);
      fwrite (".c26", sizeof (unsigned char), 4, outfile);

      tag.type = VERSION;
      tag.len = 2;
      fwrite (&tag, sizeof (tag), 1, outfile);

      tag.type = WRITER;
      tag.len = strlen (writer) + 1;
      fwrite (&tag, sizeof (tag), 1, outfile);
      fwrite (writer, 1, tag.len, outfile);

      tag.type = DATA;
      tag.len = 4096;
      fwrite (&tag, sizeof (tag), 1, outfile);
      fread (&inbuf, 1, 4096, infile);
      fwrite (&inbuf, 1, 4096, outfile);

      fclose (outfile);
      fclose (infile);
    }
}
