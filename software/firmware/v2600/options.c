/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: options.c,v 1.7 1996/11/24 16:55:40 ahornby Exp $
******************************************************************************/

/* Command Line Option Parser */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

#include "dbg_mess.h"


#include "version.h"
/* *INDENT-OFF* */
/* Options common to all ports of x2600 */
struct BaseOptions {
  int rr;
  int tvtype;
  int lcon;
  int rcon;
  int bank;
  int magstep;
  char filename[80];
  int sound;
  int swap;
  int realjoy;
  int limit;
  int mousey;
  int mitshm;
  int dbg_level;
} base_opts={1,0,0,0,0,1,"",1,0,0,0,0,1,1};

#ifdef notdef
static void
copyright(void)
{
    printf("\nVirtual 2600 Version %s. Copyright 1995/6 Alex Hornby.\n"
"-------------------------------------------------------\n"
"Distributed under the terms of the GNU General public license.\n"
"Virtual 2600 comes with ABSOLUTELY NO WARRANTY; for details see file COPYING.\n"
"This is free software, and you are welcome to redistribute it under\n"
"certain conditions.\n\n", VERSION);
}

static void
base_usage(void)
{
#if HAVE_GETOPT_LONG
printf("usage: v2600 [options] filename\n"
"Both short and long options are accepted\n"
"where options include:\n"
"        -h --help           This information\n"
"        -v --version        Show the current version/copyright info\n"
"        -f --framerate int  Set the refresh rate (1)\n"
"        -n --ntsc           Emulate an NTSC 2600 (default)\n"
"        -p --pal            Emulate a PAL 2600\n"
"        -l --lcon  <type>   Left controller, 0=JOY, 1=PADDLE, 2=KEYPAD.\n"
"        -r --rcon <type>    Right controller, 0=JOY, 1=PADDLE, 2=KEYPAD.\n"
"        -b --bank <type>    Bank switching scheme, 0=NONE,1=Atari 8k,\n"
"                            2=Atari 16k, 3=Parker 8k, 4=CBS, 5=Atari Super Chip\n"
"        -m --magstep <int>  The magnification to use (X11 only)\n"
"        -s --sound          Turn on sound. (default)\n"
"        -S --nosound        Turn off sound.\n"
"        -w --swap           Swap the left and right control methods.\n"
"        -j --realjoy        Use real joysticks.\n"
"        -t --limit          Turn on speed limiting\n"
"        -y --mousey         Use mouse y for paddle (e.g. for video olypmics).\n"
"        -M --nomitshm       turn OFF MIT Shared memory for X11.\n"
"        -V --dbg_level      Set the debugging verbosity level, 0=NONE, 1=USER\n"
"                            2-4 programmer debug.\n"
);
#else
printf("usage: v2600 [options] filename\n"
"where options include:\n"
"        -h           This information\n"
"        -v           Show the current version/copyright info\n"
"        -f int       Set the refresh rate (1)\n"
"        -n           Emulate an NTSC 2600 (default)\n"
"        -p           Emulate a PAL 2600\n"
"        -l <type>    Left controller, 0=JOY, 1=PADDLE, 2=KEYPAD.\n"
"        -r <type>    Right controller, 0=JOY, 1=PADDLE, 2=KEYPAD.\n"
"        -b <type>    Bank switching scheme, 0=NONE,1=Atari 8k,\n"
"                     2=Atari 16k, 3=Parker 8k, 4=CBS, 5=Atari Super Chip\n"
"        -m <int>  The maginification to use (X11 only)\n"
"        -s        Turn on sound. (default)\n"
"        -S        Turn off sound.\n"
"        -w        Swap the left and right control methods.\n"
"        -j        Use real joysticks.\n"
"        -t        Turns on speed limiting\n"
"        -y        Use mouse y for paddle (e.g. for video olympics).\n"
"        -M        Don't use MIT Shared memory for X11.\n"
"        -V        Set the debugging verbosity level, 0=NONE, 1=USER\n"
"                  2-4 programmer debug.\n"
);
#endif
}

static void 
usage(void)
{
    base_usage();
}

int
parse_options(int argc, char **argv)
{  
    int c;

    while (1)
    {
	int option_index = 0;
#if HAVE_GETOPT_LONG
	static struct option long_options[] =
	{
            {"framerate", 1, 0, 'f'},
            {"ntsc", 0, 0, 'n'},
            {"pal", 0, 0, 'p'},
            {"help", 0, 0, 'h'},
            {"version", 0, 0, 'v'},
            {"lcon", 1, 0, 'l'},
            {"rcon", 1, 0, 'r'},
	    {"bank", 1, 0, 'b'},
	    {"magstep", 1, 0, 'm'},
	    {"sound", 1, 0, 's'},
	    {"nosound", 1, 0, 'S'},
	    {"swap", 1, 0, 'w'},
	    {"realjoy", 1, 0, 'j'},
	    {"limit", 1, 0, 't'},
	    {"mousey", 1, 0, 'y'},
	    {"nomitshm", 1, 0, 'M'},
	    {"dbg_level", 1, 0, 'V'}
	};
	
	c = getopt_long (argc, argv, "hvf:npl:r:b:m:sSwjtyMV:",
			 long_options, &option_index);	
#else
	c = getopt(argc, argv, "hvf:npl:r:b:m:sSwjtyMV:");
#endif  /* GETOPT_LONG */
	if (c == -1)
            break;

	switch (c)
	{
	case 'h':
	    copyright();
	    usage();
	    exit(0);
	    break;

	case 'v':
	    copyright();
	    exit(0);
	    break;

	case 'f':
	    printf ("Setting frame rate to `%s'\n", optarg);
	    base_opts.rr=atoi(optarg);
	    break;
	    
	case 'n':
	    base_opts.tvtype=0;
	    break;

	case 'p':
	    base_opts.tvtype=1;
	    break;

	case 'l':
	    printf ("option l with value `%s'\n", optarg);
	    base_opts.lcon=atoi(optarg);
	    break;
	
	case 'r':
	    printf ("option r with value `%s'\n", optarg);
	    base_opts.rcon=atoi(optarg);
	    break;

	case 'b':
	    printf ("option b with value `%s'\n", optarg);
	    base_opts.bank=atoi(optarg);
	    break;

	case 'm':
	    printf ("option m with value `%s'\n", optarg);
	    base_opts.magstep=atoi(optarg);
	    break;
	    
	case 's':
	    base_opts.sound=1;
	    break;

	case 'S':
	    base_opts.sound=0;
	    break;

	case 'w':
	    base_opts.swap=1;
	    break;

	case 'j':
	    base_opts.realjoy=1;
	    break;

	case 't':
	    base_opts.limit=1;
	    break;

	case 'y':
	    base_opts.mousey=1;
	    break;

	case 'M':
	    base_opts.mitshm=0;
	    break;

	case 'V':
	    base_opts.dbg_level=atoi(optarg);
	    break;

	default:
	    fprintf (stderr,"\ngetopt() returned character code 0%o\n", c);
	}
    }
    if (optind < argc)
    {
	dbg_message(DBG_NORMAL, "non-option ARGV-elements: ");
	while (optind < argc) {
	    dbg_message(DBG_NORMAL, "%s\n",argv[optind]);
	    if(argv[optind]!=NULL)
		strcpy(base_opts.filename, argv[optind]);
	    optind++;
	}
    }
    return 0;
}
#endif

