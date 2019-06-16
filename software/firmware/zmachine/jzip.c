
/* $Id: jzip.c,v 1.3 2000/10/04 23:07:57 jholder Exp $   
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information   
 * --------------------------------------------------------------------
 * 
 * File name: $Id: jzip.c,v 1.3 2000/10/04 23:07:57 jholder Exp $  
 *   
 * Description:    
 *    
 * Modification history:      
 * $Log: jzip.c,v $
 * Revision 1.3  2000/10/04 23:07:57  jholder
 * fixed redirect problem with isolatin1 range chars
 *
 * Revision 1.2  2000/05/25 22:28:56  jholder
 * changes routine names to reflect zmachine opcode names per spec 1.0
 *
 * Revision 1.1.1.1  2000/05/10 14:21:34  jholder
 *
 * imported
 *
 *
 * --------------------------------------------------------------------
 */

/*
 * jzip.c
 *
 * Z code interpreter main routine. 
 *
 * Revisions list:
 * Mark Howell 10-Mar-1993 V2.0    howell_ma@movies.enet.dec.com
 *                         V2.0.1(some of a-f)
 * John Holder 22-Nov-1995 V2.0.1g j-holder@home.com
 * John Holder 06-Feb-1998 V2.0.2  j-holder@home.com Zstrict & Quetzal
 *
 * If you have problems with this interpreter and/or fix bugs in it, 
 * please notify John Holder (j-holder@home.com) and he will add your 
 * fix to the official JZIP distribution.
 */

#include "ztypes.h"
#include "jzexe.h"

extern int GLOBALVER;

#ifdef notused

static void configure( zbyte_t, zbyte_t );

/*
 * main
 *
 * Initialise environment, start interpreter, clean up.
 */

int _main( int argc, char *argv[] )
{
   process_arguments( argc, argv );

   configure( V1, V8 );

   initialize_screen(  );

   load_cache(  );

   z_restart(  );

   ( void ) interpret(  );

   unload_cache(  );

   close_story(  );

   close_script(  );

   reset_screen(  );

   //exit( EXIT_SUCCESS );
   while(1);

   return ( 0 );

}                               /* main */

/*
 * configure
 *
 * Initialise global and type specific variables.
 *
 */

static void configure( zbyte_t min_version, zbyte_t max_version )
{
   zbyte_t header[PAGE_SIZE], second;

   read_page( 0, header );
   datap = header;

   h_type = get_byte( H_TYPE );

   if ( h_type == 'M' || h_type == 'Z' )
   {
      /* possibly a DOS executable file, look more closely. (mol951115) */
      second = get_byte( H_TYPE + 1 );
      if ( ( h_type == 'M' && second == 'Z' ) || ( h_type == 'Z' && second == 'M' ) )
         if ( analyze_exefile(  ) )
         {
            /* Bingo!  File is a standalone game file.  */
            /* analyze_exefile has updated story_offset, so now we can */
            /* read the _real_ first page, and continue as if nothing  */
            /* has happened. */
            read_page( 0, header );
            h_type = get_byte( H_TYPE );
         }
   }

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

}                               /* configure */
#endif
