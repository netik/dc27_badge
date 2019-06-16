#include <stdio.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"

#include "ff.h"
#include "ffconf.h"

#include "badge.h"
#include "ztypes.h"

#define MAXFILELIST 50 // max. # of game files to display

static char **storyfilelist;

extern void configure( zbyte_t, zbyte_t );

extern ztheme_t themes[];
int theme = 2; // default theme

static int selectTheme(void);
static int selectStory(void);

static int selectTheme(void)
{
  int buflen = 100;
  char buf[100];
  static int theme = 1;
  int timeout = 1;
  int read_size = 0;
  buf[0] = '\0';
  extern int themecount;

  do
    {
      for(int i = 1; i <= themecount; i++)
	{
	  printf("%d: %s\n", i, themes[i-1].tname);
	}

      printf("Choose a color theme by number [%d]: ", theme);
      fflush (stdout);
      input_line( buflen, buf, timeout, &read_size );

      if (read_size > 0)
	{
	  // use entered value, otherwise, use default
	  buf[read_size] = '\0';
	  theme = atoi(buf);
	}
    }
  
  while(theme < 1 || theme > themecount);

  return theme - 1;
}

static int selectStory(void)
{
  static int storynum = 1;
  int count = 0;
  do
    {
      char buffer[100];
      int buflen = 100;
      int timeout = 0;
      int read_size = 0;
      count = 0;

      while(storyfilelist[count] != NULL && count < MAXFILELIST)
	{
	  printf("%d: %s\n", count + 1, storyfilelist[count]);
	  count++;
	}
      if(count == 0)
	{
	  printf ("No stories found\n");
          return (-1);
	}
      printf("Choose a story by number [%d]:", storynum);
      fflush (stdout);

      input_line( buflen, buffer, timeout, &read_size );
      if(read_size > 0)
	{
	  buffer[read_size] = '\0';
	  storynum = atoi(buffer);
	}
      printf("\n");
    }
  while(storynum <= 0 || storynum > count);
  
  return storynum - 1;
}

char **getDirectory(char *dirname)
{
  static char filebuff[MAXFILELIST*32];
  char *filebuffptr = filebuff;
  static char* dirlist[MAXFILELIST]; // max file list size
  int dirlistcount = 0;

  DIR d;
  FILINFO info;

  //clear previous filelist
  for(int i = 0; i < MAXFILELIST; i++)
    {
      dirlist[i] = NULL;
    }

  f_opendir(&d, dirname);

  while (true)
    {
      if (f_readdir (&d, &info) != FR_OK || info.fname[0] == 0)
	break;

      if (! (info.fattrib & AM_DIR))
	{
	  dirlist[dirlistcount++] = filebuffptr;
	  strcpy(filebuffptr, info.fname);
	  filebuffptr += strlen(info.fname) + 1;

	  if (dirlistcount > MAXFILELIST)
	    {
	      // no more files
	      break;
	    }
	}
    }
  
  f_closedir (&d);

  return dirlist;
}

static void
cmd_xyzzy (BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argv;
  int storynum;
  char storyfile[200];

  // handle arg here if necessary
  printf("\nzmachine!\n\n");

  // prompt for theme
  theme = selectTheme();

  //initialize_screen();

  // prompt for story file
  storyfilelist = getDirectory(GAMEPATH);
  storynum = selectStory();

  if (storynum == -1)
      return;

  sprintf(storyfile,"%s/%s",GAMEPATH, storyfilelist[storynum]);

  printf("\nOpening story...\n");

  open_story(storyfile);
  configure((zbyte_t) V1, (zbyte_t) V8 );
  initialize_screen(  );
  load_cache();
  z_restart(  );
  ( void ) interpret(  );
  close_script(  );
  close_story(  );
  unload_cache(  );
  reset_screen(  );

  printf("Thanks for playing!\n");

  /*
   * This is a small tweak to reclaim some memory from the heap.
   * when using getc()/getchar(), the newlib stdio code will allocate
   * some buffer memory for the stdin FILE pointer. It's about 1024
   * bytes, and it would be nice to reclaim it. Doing an fclose()
   * on stdin has the side effect of doing exactly that.
   */

  fclose (stdin);

  return;
}

orchard_command("xyzzy", cmd_xyzzy);
