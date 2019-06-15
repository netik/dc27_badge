
/* $Id: osdepend.c,v 1.2 2000/05/25 22:28:56 jholder Exp $   
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information   
 * --------------------------------------------------------------------
 * 
 * File name: $Id: osdepend.c,v 1.2 2000/05/25 22:28:56 jholder Exp $  
 *   
 * Description:    
 *    
 * Modification history:      
 * $Log: osdepend.c,v $
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
 * osdepend.c
 *
 * All non screen specific operating system dependent routines.
 *
 * Olaf Barthel 28-Jul-1992
 * Modified John Holder(j-holder@home.com) 25-July-1995
 * Support for standalone storyfiles by Magnu Olsson (mol@df.lth.se) Nov.1995
 *
 */

#include "ztypes.h"
#include "jzexe.h"

/* File names will be O/S dependent */

#if defined(AMIGA)
#define SAVE_NAME     "Story.Save" /* Default save name */
#define SCRIPT_NAME   "PRT:"    /* Default script name */
#define RECORD_NAME   "Story.Record" /* Default record name */
#define AUXILARY_NAME "Story.Aux" /* Default auxilary name */
#else /* defined(AMIGA) */
#define SAVE_NAME     "story.sav" /* Default save name */
#define SCRIPT_NAME   "story.scr" /* Default script name */
#define RECORD_NAME   "story.rec" /* Default record name */
#define AUXILARY_NAME "story.aux" /* Default auxilary name */
#endif /* defined(AMIGA) */


#ifdef STRICTZ

/* Define stuff for stricter Z-code error checking, for the generic
 * Unix/DOS/etc terminal-window interface. Feel free to change the way
 * player prefs are specified, or replace report_zstrict_error()
 * completely if you want to change the way errors are reported. 
 */

/* There are four error reporting modes: never report errors;
 * report only the first time a given error type occurs; report
 * every time an error occurs; or treat all errors as fatal
 * errors, killing the interpreter. I strongly recommend
 * "report once" as the default. But you can compile in a
 * different default by changing the definition of
 * STRICTZ_DEFAULT_REPORT_MODE. In any case, the player can
 * specify a report mode on the command line by typing "-s 0"
 * through "-s 3". 
 */

#define STRICTZ_REPORT_NEVER  (0)
#define STRICTZ_REPORT_ONCE   (1)
#define STRICTZ_REPORT_ALWAYS (2)
#define STRICTZ_REPORT_FATAL  (3)

#define STRICTZ_DEFAULT_REPORT_MODE STRICTZ_REPORT_NEVER

static int strictz_report_mode;
static int strictz_error_count[STRICTZ_NUM_ERRORS];

#endif /* STRICTZ */


#if !defined(AMIGA)

/* getopt linkages */

extern int optind;
extern const char *optarg;
extern ZINT16 default_fg, default_bg;

#endif /* !defined(AMIGA) */


#if defined OS2 || defined __MSDOS__ 
int iPalette;                   
#endif 



#if !defined(AMIGA)

/*
 * process_arguments
 *
 * Do any argument preprocessing necessary before the game is
 * started. This may include selecting a specific game file or
 * setting interface-specific options.
 *
 */

void process_arguments( int argc, char *argv[] )
{
   int c, errflg = 0;
   int infoflag = 0;
   int size;
   int expected_args;
   int num;

#ifdef STRICTZ
   /* Initialize the STRICTZ variables. */

   strictz_report_mode = STRICTZ_DEFAULT_REPORT_MODE;

   for ( num = 0; num < STRICTZ_NUM_ERRORS; num++ )
   {
      strictz_error_count[num] = 0;
   }
#endif /* STRICTZ */

   /* Parse the options */

   monochrome = 0;
   hist_buf_size = 1024;
   bigscreen = 0;

#ifdef STRICTZ                  
#if defined OS2 || defined __MSDOS__ 
#define GETOPT_SET "gbomvzhy?l:c:k:r:t:s:" 
#elif defined TURBOC            
#define GETOPT_SET   "bmvzhy?l:c:k:r:t:s:" 
#elif defined HARD_COLORS       
#define GETOPT_SET    "mvzhy?l:c:k:r:t:s:f:b:" 
#else 
#define GETOPT_SET    "mvzhy?l:c:k:r:t:s:" 
#endif 
#else 
#if defined OS2 || defined __MSDOS__ 
#define GETOPT_SET "gbomvzhy?l:c:k:r:t:" 
#elif defined TURBOC            
#define GETOPT_SET   "bmvzhy?l:c:k:r:t:" 
#elif defined HARD_COLORS       
#define GETOPT_SET    "mvzhy?l:c:k:r:t:f:b:" 
#else 
#define GETOPT_SET    "mvzhy?l:c:k:r:t:" 
#endif 
#endif 
   while ( ( c = getopt( argc, argv, GETOPT_SET ) ) != EOF )
   {                            
      switch ( c )
      {
         case 'l':             /* lines */
            screen_rows = atoi( optarg );
            break;
         case 'c':             /* columns */
            screen_cols = atoi( optarg );
            break;
         case 'r':             /* right margin */
            right_margin = atoi( optarg );
            break;
         case 't':             /* top margin */
            top_margin = atoi( optarg );
            break;
         case 'k':             /* number of K for hist_buf_size */
            size = atoi( optarg );
            hist_buf_size = ( hist_buf_size > size ) ? hist_buf_size : size;
            if ( hist_buf_size > 16384 )
               hist_buf_size = 16384;
            break;
         case 'y':             /* Tandy */
            fTandy = 1;         
            break;              
#if defined OS2 || defined __MSDOS__ 
         case 'g':             /* Beyond Zork or other games using IBM graphics */
            fIBMGraphics = 1;   
            break;              
#endif 
         case 'v':             /* version information */

            fprintf( stdout, "\nJZIP - An Infocom Z-code Interpreter Program \n" );
            fprintf( stdout, "       %s %s\n", JZIPVER, JZIPRELDATE );
            if ( STANDALONE_FLAG )
            {
               fprintf( stdout, "       Standalone game: %s\n", argv[0] );
            }
            fprintf( stdout, "---------------------------------------------------------\n" );
            fprintf( stdout, "Author          :  %s\n", JZIPAUTHOR );
            fprintf( stdout, "Official Webpage: %s\n", JZIPURL );
            fprintf( stdout, "IF Archive      : ftp://ftp.gmd.de/if-archive/interpreters/zip/\n" );
            fprintf( stdout, "         Based on ZIP 2.0 source code by Mark Howell\n\n" );
            fprintf( stdout,
                     "Bugs:    Please report bugs and portability bugs to the maintainer." );
            fprintf( stdout, "\n\nInterpreter:\n\n" );
            fprintf( stdout, "\tThis interpreter will run all Infocom V1 to V5 and V8 games.\n" );
            fprintf( stdout,
                     "\tThis is a Z-machine standard 1.0 interpreter, including support for\n" );
            fprintf( stdout, "\tthe Quetzal portable save file format, ISO 8859-1 (Latin-1)\n" );
            fprintf( stdout,
                     "\tinternational character support, and the extended save and load opcodes.\n" );
            fprintf( stdout, "\t\n" );
            infoflag++;
            break;
#if defined (HARD_COLORS)
         case 'f':
            default_fg = atoi( optarg );
            break;
         case 'b':
            default_bg = atoi( optarg );
            break;
#endif
#if defined OS2 || defined __MSDOS__ 
         case 'm':             /* monochrome */
            iPalette = 2;       
            break;              
         case 'b':             /* black-and-white */
            iPalette = 1;       
            break;              
         case 'o':             /* color */
            iPalette = 0;       
            break;              
#else 
         case 'm':
            monochrome = 1;
            break;
#endif 
#if defined TURBOC              
         case 'b':
            bigscreen = 1;
            break;
#endif 
#ifdef STRICTZ
         case 's':             /* strictz reporting mode */
            strictz_report_mode = atoi( optarg );
            if ( strictz_report_mode < STRICTZ_REPORT_NEVER ||
                 strictz_report_mode > STRICTZ_REPORT_FATAL )
            {
               errflg++;
            }
            break;
#endif /* STRICTZ */

         case 'z':
            print_license(  );
            infoflag++;
            break;

         case 'h':
         case '?':
         default:
            errflg++;
      }
   }

   if ( infoflag )
      while(1); // need to fix this
      //exit( EXIT_SUCCESS );

   if ( STANDALONE_FLAG )
      expected_args = 0;
   else
      expected_args = 1;

   /* Display usage */

   if ( errflg || optind + expected_args != argc )
   {

      if ( STANDALONE_FLAG )
         fprintf( stdout, "usage: %s [options...]\n", argv[0] ); 
      else
         fprintf( stdout, "usage: %s [options...] story-file\n", argv[0] ); 

      fprintf( stdout, "JZIP - an Infocom story file interpreter.\n" );
      fprintf( stdout, "      Version %s by %s.\n", JZIPVER, JZIPAUTHOR );
      fprintf( stdout, "      Release %s.\n", JZIPRELDATE );
      fprintf( stdout, "      Based on ZIP V2.0 source by Mark Howell\n" );
      fprintf( stdout, "      Plays types 1-5 and 8 Infocom and Inform games.\n\n" );
      fprintf( stdout, "\t-l n lines in display\n" );
      fprintf( stdout, "\t-c n columns in display\n" );
      fprintf( stdout, "\t-r n text right margin (default = %d)\n", DEFAULT_RIGHT_MARGIN );
      fprintf( stdout, "\t-t n text top margin (default = %d)\n", DEFAULT_TOP_MARGIN );
      fprintf( stdout, "\t-k n set the size of the command history buffer to n bytes\n" );
      fprintf( stdout, "\t     (Default is 1024 bytes.  Maximum is 16384 bytes.)\n" );
      fprintf( stdout, "\t-y   turn on the legendary \"Tandy\" bit\n" ); 
      fprintf( stdout, "\t-v   display version information\n" );
      fprintf( stdout, "\t-h   display this usage information\n" );

#if defined (HARD_COLORS)
      fprintf( stdout, "\t-f n foreground color\n" );
      fprintf( stdout, "\t-b n background color (-1 to ignore bg color (try it on an Eterm))\n" );
      fprintf( stdout, "\t     Black=0 Red=1 Green=2 Yellow=3 Blue=4 Magenta=5 Cyan=6 White=7\n" );
#endif
      fprintf( stdout, "\t-m   force monochrome mode\n" ); 
#if defined __MSDOS__ || defined OS2 
      fprintf( stdout, "\t-b   force black-and-white mode\n" ); 
      fprintf( stdout, "\t-o   force color mode\n" ); 
      fprintf( stdout, "\t-g   use \"Beyond Zork\" graphics, rather than standard international\n" ); 
#elif defined TURBOC            
      fprintf( stdout, "\t-b   run in 43/50 line EGA/VGA mode\n" ); 
#endif 

#ifdef STRICTZ
      fprintf( stdout, "\t-s n stricter error checking (default = %d) (0: none; 1: report 1st\n",
               STRICTZ_DEFAULT_REPORT_MODE );
      fprintf( stdout, "\t     error; 2: report all errors; 3: exit after any error)\n" );
#endif /* STRICTZ */

      fprintf( stdout, "\t-z   display license information.\n" );
      exit( EXIT_FAILURE );
   }

   /* Open the story file */

   if ( !STANDALONE_FLAG )      /* mol 951115 */
      open_story( argv[optind] );
   else
   {
      /* standalone, ie. the .exe file _is_ the story file. */
      if ( argv[0][0] == 0 )
      {
         /* some OS's (ie DOS prior to v.3.0) don't supply the path to */
         /* the .exe file in argv[0]; in that case, we give up. (mol) */
         fatal( "process_arguments(): Sorry, this program will not run on this platform." );
      }
      open_story( argv[0] );
   }

}                               /* process_arguments */

#endif /* !defined(AMIGA) */

#if !defined(AMIGA)

/*
 * file_cleanup
 *
 * Perform actions when a file is successfully closed. Flag can be one of:
 * GAME_SAVE, GAME_RESTORE, GAME_SCRIPT.
 *
 */

void file_cleanup( const char *file_name, int flag )
{
   UNUSEDVAR( file_name );
   UNUSEDVAR( flag );
}                               /* file_cleanup */

#endif /* !defined(AMIGA) */

#if !defined(AMIGA)

/*
 * sound
 *
 * Play a sound file or a note.
 *
 * argc = 1: argv[0] = note# (range 1 - 3)
 *
 *           Play note.
 *
 * argc = 2: argv[0] = 0
 *           argv[1] = 3
 *
 *           Stop playing current sound.
 *
 * argc = 2: argv[0] = 0
 *           argv[1] = 4
 *
 *           Free allocated resources.
 *
 * argc = 3: argv[0] = ID# of sound file to replay.
 *           argv[1] = 2
 *           argv[2] = Volume to replay sound with, this value
 *                     can range between 1 and 8.
 *
 * argc = 4: argv[0] = ID# of sound file to replay.
 *           argv[1] = 2
 *           argv[2] = Control information
 *           argv[3] = Volume information
 *
 *           Volume information:
 *
 *               0x34FB -> Fade sound in
 *               0x3507 -> Fade sound out
 *               other  -> Replay sound at maximum volume
 *
 *           Control information:
 *
 *               This word is divided into two bytes,
 *               the upper byte determines the number of
 *               cycles to play the sound (e.g. how many
 *               times a clock chimes or a dog barks).
 *               The meaning of the lower byte is yet to
 *               be discovered :)
 *
 */

void sound( int argc, zword_t * argv )
{

   /* Supply default parameters */

   if ( argc < 4 )
      argv[3] = 0;
   if ( argc < 3 )
      argv[2] = 0xff;
   if ( argc < 2 )
      argv[1] = 2;

   /* Generic bell sounder */

   if ( argc == 1 || argv[1] == 2 )
      display_char( '\007' );

}                               /* sound */

#endif /* !defined(AMIGA) */

/*
 * get_file_name
 *
 * Return the name of a file. Flag can be one of:
 *    GAME_SAVE     - Save file (write only)
 *    GAME_RESTORE  - Save file (read only)
 *    GAME_SCRIPT   - Script file (write only)
 *    GAME_RECORD   - Keystroke record file (write only)
 *    GAME_PLABACK  - Keystroke record file (read only)
 *    GAME_SAVE_AUX - Auxilary (preferred settings) file (write only)
 *    GAME_LOAD_AUX - Auxilary (preferred settings) file (read only)
 */

int get_file_name( char *file_name, char *default_name, int flag )
{
   char buffer[127 + 2];        /* 127 is the biggest positive char */
   int status = 0;

   /* If no default file name then supply the standard name */

   if ( default_name[0] == '\0' )
   {
      if ( flag == GAME_SCRIPT )
         strcpy( default_name, SCRIPT_NAME );
      else if ( flag == GAME_RECORD || flag == GAME_PLAYBACK )
         strcpy( default_name, RECORD_NAME );
      else if ( flag == GAME_SAVE_AUX || GAME_LOAD_AUX )
         strcpy( default_name, AUXILARY_NAME );
      else                      /* (flag == GAME_SAVE || flag == GAME_RESTORE) */
         strcpy( default_name, SAVE_NAME );
   }

   /* Prompt for the file name */

   output_line( "Enter a file name." );
   output_string( "(Default is \"" );
   output_string( default_name );
   output_string( "\"): " );

   buffer[0] = 127;
   buffer[1] = 0;
   ( void ) get_line( buffer, 0, 0 );

   /* Copy file name from the input buffer */

   if ( h_type > V4 )
   {
      unsigned char len = buffer[1];

      memcpy( file_name, &buffer[2], len );
      file_name[len] = '\0';
   }
   else
      strcpy( file_name, &buffer[1] );

   /* If nothing typed then use the default name */

   if ( file_name[0] == '\0' )
      strcpy( file_name, default_name );

#if !defined(VMS)               /* VMS has file version numbers, so cannot overwrite */

   /* Check if we are going to overwrite the file */

   if ( flag == GAME_SAVE || flag == GAME_SCRIPT || flag == GAME_RECORD || flag == GAME_SAVE_AUX )
   {
      FILE *tfp;

#if defined BUFFER_FILES        
      char tfpbuffer[BUFSIZ];   
#endif 
      char c;

      /* Try to access the file */

      tfp = fopen( file_name, "r" );
      if ( tfp != NULL )
      {
         /* If it succeeded then prompt to overwrite */

#if defined BUFFER_FILES        
         setbuf( tfp, tfpbuffer ); 
#endif 

         output_line( "You are about to write over an existing file." );
         output_string( "Proceed? (Y/N) " );

         do
         {
            c = ( char ) input_character( 0 );
            c = ( char ) toupper( c );
         }
         while ( c != 'Y' && c != 'N' );

         output_char( c );
         output_new_line(  );

         /* If no overwrite then fail the routine */

         if ( c == 'N' )
            status = 1;

         fclose( tfp );
      }
   }
#endif /* !defined(VMS) */

   /* Record the file name if it was OK */

   if ( status == 0 )
      record_line( file_name );

   return ( status );

}                               /* get_file_name */

#if !defined(AMIGA)

/*
 * fatal
 *
 * Display message and stop interpreter.
 *
 */

void fatal( const char *s )
{
   //reset_screen(  );
   char strbuff[512];
   sprintf(strbuff, "\n**FATAL ERROR: %s", s);
   printf(strbuff);
   while(1)

   }
}                               /* fatal */

#endif /* !defined(AMIGA) */

/*
 * report_strictz_error
 *
 * This handles Z-code error conditions which ought to be fatal errors,
 * but which players might want to ignore for the sake of finishing the
 * game.
 *
 * The error is provided as both a numeric code and a string. This allows
 * us to print a warning the first time a particular error occurs, and
 * ignore it thereafter.
 *
 * errnum : Numeric code for error (0 to STRICTZ_NUM_ERRORS-1)
 * errstr : Text description of error
 *
 */

#ifdef STRICTZ

void report_strictz_error( int errnum, const char *errstr )
{
   int wasfirst;
   char buf[256] = { 0 };

   if ( errnum <= 0 || errnum >= STRICTZ_NUM_ERRORS )
      return;

   if ( strictz_report_mode == STRICTZ_REPORT_FATAL )
   {
      sprintf( buf, "STRICTZ: %s (PC = %lx)", errstr, pc );
      fatal( buf );
      return;
   }

   wasfirst = ( strictz_error_count[errnum] == 0 );
   strictz_error_count[errnum]++;

   if ( ( strictz_report_mode == STRICTZ_REPORT_ALWAYS ) ||
        ( strictz_report_mode == STRICTZ_REPORT_ONCE && wasfirst ) )
   {
      sprintf( buf, "STRICTZ Warning: %s (PC = %lx)", errstr, pc );
      write_string( buf );

      if ( strictz_report_mode == STRICTZ_REPORT_ONCE )
      {
         write_string( " (will ignore further occurrences)" );
      }
      else
      {
         sprintf( buf, " (occurrence %d)", strictz_error_count[errnum] );
         write_string( buf );
      }
      z_new_line(  );
   }

}                               /* report_strictz_error */

#endif /* STRICTZ */


#if !defined(AMIGA)

/*
 * fit_line
 *
 * This routine determines whether a line of text will still fit
 * on the screen.
 *
 * line : Line of text to test.
 * pos  : Length of text line (in characters).
 * max  : Maximum number of characters to fit on the screen.
 *
 */
int fit_line( const char *line_buffer, int pos, int max )
{
   UNUSEDVAR( line_buffer );

   return ( pos < max );
}                               /* fit_line */

#endif /* !defined(AMIGA) */

#if !defined(AMIGA)

/*
 * print_status
 *
 * Print the status line (type 3 games only).
 *
 * argv[0] : Location name
 * argv[1] : Moves/Time
 * argv[2] : Score
 *
 * Depending on how many arguments are passed to this routine
 * it is to print the status line. The rendering attributes
 * and the status line window will be have been activated
 * when this routine is called. It is to return FALSE if it
 * cannot render the status line in which case the interpreter
 * will use display_char() to render it on its own.
 *
 * This routine has been provided in order to support
 * proportional-spaced fonts.
 *
 */

int print_status( int argc, char *argv[] )
{
   UNUSEDVAR( argc );
   UNUSEDVAR( argv );

   return ( FALSE );
}                               /* print_status */

#endif /* !defined(AMIGA) */

#if !defined(AMIGA)

/*
 * set_font
 *
 * Set a new character font. Font can be either be:
 *
 *    TEXT_FONT (1)     = normal text character font
 *    GRAPHICS_FONT (3) = graphical character font
 *
 */

void set_font( int font_type )
{
   UNUSEDVAR( font_type );
}                               /* set_font */

#endif /* !defined(AMIGA) */

#if !defined MSDOS && !defined OS2 && !defined AMIGA && !defined HARD_COLORS && !defined ATARIST 

/*
 * set_colours
 *
 * Sets screen foreground and background colours.
 *
 */

/*
void set_colours( zword_t foreground, zword_t background )
{

}
*/
#endif  /* !defined MSDOS && !defined OS2 && !defined AMIGA !defined HARD_COLORS && !defined ATARIST */ 

#if !defined VMS && !defined MSDOS && !defined OS2 && !defined POSIX 

/*
 * codes_to_text
 *
 * Translate Z-code characters to machine specific characters. These characters
 * include line drawing characters and international characters.
 *
 * The routine takes one of the Z-code characters from the following table and
 * writes the machine specific text replacement. The target replacement buffer
 * is defined by MAX_TEXT_SIZE in ztypes.h. The replacement text should be in a
 * normal C, zero terminated, string.
 *
 * Return 0 if a translation was available, otherwise 1.
 *
 *  Arrow characters (0x18 - 0x1b):
 *
 *  0x18 Up arrow
 *  0x19 Down arrow
 *  0x1a Right arrow
 *  0x1b Left arrow
 *
 *  International characters (0x9b - 0xa3):
 *
 *  0x9b a umlaut (ae)
 *  0x9c o umlaut (oe)
 *  0x9d u umlaut (ue)
 *  0x9e A umlaut (Ae)
 *  0x9f O umlaut (Oe)
 *  0xa0 U umlaut (Ue)
 *  0xa1 sz (ss)
 *  0xa2 open quote (>>)
 *  0xa3 close quota (<<)
 *
 *  Line drawing characters (0xb3 - 0xda):
 *
 *  0xb3 vertical line (|)
 *  0xba double vertical line (#)
 *  0xc4 horizontal line (-)
 *  0xcd double horizontal line (=)
 *  all other are corner pieces (+)
 *
 */

/*
int codes_to_text( int c, char *s )
{
   return 1;
}
*/
#endif  /* !defined VMS && !defined MSDOS && !defined OS2 && !defined POSIX */ 
