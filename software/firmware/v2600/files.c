

/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: files.c,v 1.18 1997/02/23 20:39:29 ahornby Exp ahornby $
******************************************************************************/

/*
   Used to load cartridge images into memory.
 */

#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "options.h"
#include "types.h"
#include "vmachine.h"
#include "c26def.h"
#include "okie.h"
#include "dbg_mess.h"

/* Return the size of the file pointed to by fp */
/* Avoids need for UNIX type stat, leaves file */
/* pointer untouched. */
long
filesize (FILE * fp)
{
  long curpos, length;

  curpos = ftell (fp);
  fseek (fp, 0L, SEEK_END);
  length = ftell (fp);
  fseek (fp, curpos, SEEK_SET);
  return length;
}

/* Load from the common 2600 format */
/* Uses the tag structure from c26def.h */
int
loadc26 (FILE * fp, long flen)
{
  char databuf[65535];
  struct c26_tag tag;

  while (fread (&tag, sizeof (struct c26_tag), 1, fp))
    {
      /* If positive then it has a data block */
      if (tag.type >= 0)
	{
	  if (fread (databuf, sizeof (BYTE), tag.len, fp))
	    {
	      perror ("v2600 loadc26:");
	      fprintf (stderr, "Error reading data.\n");
	    }
	}

      switch (tag.type)
	{

	case VERSION:
	  if (Verbose)
	    printf ("File version %d\n", tag.len);
	  break;

	case WRITER:
	  if (Verbose)
	    printf ("Written by: %s\n", databuf);
	  break;

	case TVTYPE:
	  base_opts.tvtype = tag.len;
	  break;

	case CONTROLLERS:
	  /* Higher byte */
	  base_opts.lcon = (tag.len & 0xff00)>>8;
	  /* Low byte */
	  base_opts.rcon = (tag.len) & 0xff;

	case BANKING:
	  base_opts.bank = tag.len;
	  break;

	case DATA:
	  if (tag.len <= 16384)
	    memcpy (&theCart[0], databuf, tag.len);
	  else
	    {
	      fprintf (stderr, "Data larger that 16k!\n");
	      exit (-1);
	    }
	  rom_size = tag.len;
	  if (tag.len == 2048)
	    {
	      memcpy (&theCart[0], databuf, tag.len);
	      memcpy (&theCart[2048], databuf, tag.len);
	    }
	  break;

	default:
	  fprintf (stderr, "Unknown tag %d\n. Ignoring.", tag.type);
	  break;
	}
    }
  return 0;
}

/* Load a raw binary image  */
/* fp: file stream to load */
/* stat_data: unix stat structure for file pointed to by fp */
/* returns: size of file loaded */
int
loadRaw (FILE * fp, long flen)
{
  int size = flen;
  if (Verbose)
    printf ("Raw loading %d bytes\n", size);

  if (size > 16384)
    size = 16384;
  fread (theCart, sizeof (BYTE), size, fp);

  rom_size = size;
  if (size == 2048)
    {
      memcpy (&theCart[2048], &theCart[0], 2048);
      rom_size = 4096;
    }
  else if (size < 2048)
    {
      theCart[0x0ffc] = 0x00;
      theCart[0x0ffd] = 0xf0;
      rom_size = 4096;
    }
  
  return size;
}

/* Load a default game */
void
load_okie_dokie(void)
{

  dbg_message (1, "No file specified, Running Builtin Okie Dokie!\n");

  memcpy (&theCart[0], &okie_dokie_data[0], 2048);
  rom_size = 2048;
  memcpy (&theCart[2048], &theCart[0], 2048);
  rom_size = 4096;  
}

/* Load a commodore format .prg file  */
/* fp: file stream to load */
/* stat_data: unix stat structure for file pointed to by fp */
/* returns: size of file loaded */
int
loadPrg (FILE * fp, long flen)
{
  int start;
  int rompos;
  int size;
  unsigned char buf1, buf2;

  /* Get the load address */
  fread (&buf1, sizeof (BYTE), 1, fp);
  fread (&buf2, sizeof (BYTE), 1, fp);
  start = buf2 * 256 + buf1;

  /* Translate the load address to a ROM address */
  rompos = start & 0xfff;

  /* Get length of data part */
  size = flen - 2;

  if (Verbose)
    printf ("Loading .prg at %x\n", start);

  /* Load the data */
  fread (&theCart[rompos], sizeof (BYTE), size, fp);

  /* Put the load address into the reset vector */
  theCart[0x0ffc] = buf1;
  theCart[0x0ffd] = buf2;
  return size;
}

/* Load a cartridge image */
/* name: filename to load */
/* returns: -1 on error 0 otherwise  */
int
loadCart (void)
{
  FILE *fp;
  char *ext;
  int flen;
  char *name = base_opts.filename;

  if (name == NULL || strcmp("",name)==0)
    {
      load_okie_dokie();
      return 0;
    }

  fp = fopen (name, "rb");
  if(!fp) {
      fprintf (stderr, "Can't find %s\n", name);
      return -1;
  }

  if (Verbose)
    printf ("Loading cart: %s\n", name);
  flen = filesize (fp);

  ext = strrchr (name, '.');
  if(!ext) {
      ext=".bin";
  }
  
  if (strcmp (".pal", ext) == 0 || strcmp (".vcs", ext) == 0 ||
      strcmp (".raw", ext) == 0 || strcmp (".bin", ext) == 0 ||
      strcmp (".BIN", ext) == 0)
    {
      int size=loadRaw (fp, flen);
      /* try and correct for idiots */
      if(base_opts.bank==0) {	
	switch(size) {
	case 8192:
	  base_opts.bank=1;
	  break;
	case 16384:
	  base_opts.bank=2;
	  break;
	default:
	  /* Do nothing */
	}
      }
    }
  else if (strcmp (".prg", ext) == 0)
    {
      loadPrg (fp, flen);
    }
  else if (strcmp (".c26", ext) == 0)
    {
      loadc26 (fp, flen);
    }
  else
    {
      fprintf (stderr, "Unknown file format %s\n", ext);
      return -1;
    }
  fclose (fp);
  return 0;
}
