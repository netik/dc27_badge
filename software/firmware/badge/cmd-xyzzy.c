#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"

#include "ff.h"
#include "ffconf.h"

#include "badge.h"
#include "zmachine/ztypes.h"

#define MAXFILELIST 50 // max. # of game files to display

static char **storyfilelist;

extern ztheme_t themes[];
static int theme = 2; // default theme

static int selectTheme(BaseSequentialStream *);
static int selectStory(BaseSequentialStream *);

static int selectTheme(BaseSequentialStream *chp)
{
  int buflen = 100;
  char buf[buflen];
  static int theme = 1;
  int timeout = 0;
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

static int selectStory(BaseSequentialStream *chp)
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
	  fatal("No stories found\n");
	}
      printf("Choose a story by number [%d]:", storynum);

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

static void configure( zbyte_t min_version, zbyte_t max_version )
{
  zbyte_t header[PAGE_SIZE], second;
  
  read_page( 0, header );
  datap = header;
  
  h_type = get_byte( H_TYPE );
  
  GLOBALVER = h_type;
  if ( h_type < min_version || h_type > max_version ||
       ( get_byte( H_CONFIG ) & CONFIG_BYTE_SWAPPED ) )
    fatal( "Wrong game or version" );
  /*
   * if (h_type == V6 || h_type == V7)
   * fatal ("Unsupported zcode version.");
   */
  
  if ( h_type < V4 )
    {
      story_scaler = 2;
      story_shift = 1;
      property_mask = P3_MAX_PROPERTIES - 1;
      property_size_mask = 0xe0;
    }
  else if ( h_type < V8 )
    {
      story_scaler = 4;
      story_shift = 2;
      property_mask = P4_MAX_PROPERTIES - 1;
      property_size_mask = 0x3f;
    }
  else
    {
      story_scaler = 8;
      story_shift = 3;
      property_mask = P4_MAX_PROPERTIES - 1;
      property_size_mask = 0x3f;
    }
  
  h_config = get_byte( H_CONFIG );
  h_version = get_word( H_VERSION );
  h_data_size = get_word( H_DATA_SIZE );
  h_start_pc = get_word( H_START_PC );
  h_words_offset = get_word( H_WORDS_OFFSET );
  h_objects_offset = get_word( H_OBJECTS_OFFSET );
  h_globals_offset = get_word( H_GLOBALS_OFFSET );
  h_restart_size = get_word( H_RESTART_SIZE );
  h_flags = get_word( H_FLAGS );
  h_synonyms_offset = get_word( H_SYNONYMS_OFFSET );
  h_file_size = get_word( H_FILE_SIZE );
  if ( h_file_size == 0 )
    h_file_size = get_story_size(  );
  h_checksum = get_word( H_CHECKSUM );
  h_alternate_alphabet_offset = get_word( H_ALTERNATE_ALPHABET_OFFSET );
  
  if ( h_type >= V5 )
    {
      h_unicode_table = get_word( H_UNICODE_TABLE );
    }
  datap = NULL;
  
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
  theme = selectTheme(chp);

  //initialize_screen();

  // prompt for story file
  storyfilelist = getDirectory(GAMEPATH);
  storynum = selectStory(chp);

  sprintf(storyfile,"%s/%s",GAMEPATH, storyfilelist[storynum]);

  printf("\nOpening story...\n");

  open_story(storyfile);
  configure((zbyte_t) V1, (zbyte_t) V8 );
  /*
  initialize_screen(  );
  load_cache();
  z_restart(  );
  ( void ) interpret(  );
  unload_cache(  );
  close_story(  );
  close_script(  );
  reset_screen(  );
  printf("Thanks for playing!\n");
  */

  return;
}

orchard_command("xyzzy", cmd_xyzzy);
