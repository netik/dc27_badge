

/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for Details.
   
   $Id: profile.c,v 1.4 1996/08/26 15:04:20 ahornby Exp $
******************************************************************************/

/* Instruction usage profiling */

#ifdef PROFILE
#include <stdio.h>
#include <stdlib.h>
#include "types.h"


/* A counter for each possible opcode */
long profile_count[256];

/* Set up cpu instruction profiling */
void
profile_init (void)
{
  int i;
  for (i = 0; i < 256; i++)
    profile_count[i] = 0;
}

/* The elements used for sorting the final results */
struct Profstruct
{
  BYTE ins;
  long count;
};

/* The compare function for sorting */
static int
pfscompare (struct Profstruct *a, struct Profstruct *b)
{
  if (a->count == b->count)
    return 0;
  else if (a->count < b->count)
    return -1;
  else
    return 1;
}

/* Print a profiling report to standard output */
void
profile_report (void)
{
  int i;
  struct Profstruct pfs[256];

  /* Construct an array of Profstructs */
  for (i = 0; i < 256; i++)
    {
      pfs[i].ins = i;
      pfs[i].count = profile_count[i];
    }

  /* Use library qsort */
  qsort (pfs, 256, sizeof (pfs[0]), pfscompare);

  /* Print the report */
  printf ("++Profile Report++\n");
  for (i = 0; i < 256; i++)
    printf ("INS:%d , COUNT: %ld \n", pfs[i].ins, pfs[i].count);
  printf ("++End Profile Report++\n");
}

#endif
