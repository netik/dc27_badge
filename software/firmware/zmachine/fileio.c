
/* $Id: fileio.c,v 1.3 2000/07/05 15:20:34 jholder Exp $   
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information   
 * --------------------------------------------------------------------
 * 
 * File name: $Id: fileio.c,v 1.3 2000/07/05 15:20:34 jholder Exp $  
 *   
 * Description:    
 *    
 * Modification history:      
 * $Log: fileio.c,v $
 * Revision 1.3  2000/07/05 15:20:34  jholder
 * Updated code to remove warnings.
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
 * fileio.c
 *
 * File manipulation routines. Should be generic.
 *
 */

#include "ztypes.h"
#include "jzexe.h"              /* mol 951115 */

/* Static data */

extern int GLOBALVER;

#ifdef USE_ZLIB
static gzFile *gfp = NULL;      /* Zcode file pointer */
#else
static FILE *gfp = NULL;        /* Zcode file pointer */
#endif

static FILE *sfp = NULL;        /* Script file pointer */
static FILE *rfp = NULL;        /* Record file pointer */

#if defined BUFFER_FILES        
#ifndef USE_ZLIB
static char gfpbuffer[BUFSIZ];  
#endif
static char sfpbuffer[BUFSIZ];  
static char rfpbuffer[BUFSIZ];  
#endif 

char save_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1] = "story.sav";
char script_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1] = "story.scr";
char record_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1] = "story.rec";
char auxilary_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1] = "story.aux";

static int undo_valid = FALSE;
static zword_t undo_stack[STACK_SIZE];

static int script_file_valid = FALSE;

static long story_offset = 0;   /* mol 951114 */
char *magic = ( char * ) MAGIC_STRING; /* mol */

static int save_restore( const char *, int );

/*
 * analyze_exefile
 *
 * This function is called if the game file seems to be a JZEXE file
 * (standalone game file). If that is the case, story_offset is set
 * to the beginning of the Z code and the function returns TRUE,
 * otherwise no action is taken and it returns FALSE.
 *
 * Magnus Olsson, November 1995
 */
int analyze_exefile( void )
{
   int c, i;

   if ( story_offset > 0 )
   {
      /* This is wrong; we shouldn't be doing this. */
      return FALSE;
   }

   /* Look for the magic string, starting from the beginning. */
   jz_rewind( gfp );
   i = 0;

   while ( ( c = jz_getc( gfp ) ) > -1 )
   {
      if ( c != magic[i] )
      {
         if ( c == magic[0] )
         {
            i = 1;              /* Next character should match magic[1] */
         }
         else
         {
            i = 0;
         }
      }
      else if ( ++i == MAGIC_END )
      {
         /* Found the magic string! The next byte must be zero. */
         if ( jz_getc( gfp ) != 0 )
         {
            return FALSE;
         }

         /* Read offset and return. We won't concern ourselves with possible
          * read errors, since their consequences will be detected later on, 
          * when we try to interpret the file as Z code. */
         story_offset = jz_getc( gfp );
         story_offset += 256L * jz_getc( gfp );
         story_offset += 65536L * jz_getc( gfp );
         return TRUE;
      }
   }
   /* If we get here, we've reached the end of the infile without success. */
   return FALSE;

}                               /* analyze_exefile */


/*
 * set_names
 *
 * Set up the story names intelligently.
 * John Holder, 28 Sept 1995
 */
void set_names( const char *storyname )
{
   char *per_pos = 0;

   strcpy( save_name, storyname );
   strcpy( script_name, storyname );
   strcpy( record_name, storyname );
   strcpy( auxilary_name, storyname );

   /* experimental setting of save_name, added by John Holder 26 July 1995 */
   per_pos = strrchr( storyname, '.' ); /* find last '.' in storyname. */
   if ( per_pos )               /* The story file looks like "Odius.dat" or "odieus.z3" */
   {
      per_pos = strrchr( save_name, '.' );
      *( per_pos ) = '\0';
      strcat( save_name, ".sav" );

      per_pos = strrchr( script_name, '.' );
      *( per_pos ) = '\0';
      strcat( script_name, ".scr" );

      per_pos = strrchr( record_name, '.' );
      *( per_pos ) = '\0';
      strcat( record_name, ".rec" );

      per_pos = strrchr( auxilary_name, '.' );
      *( per_pos ) = '\0';
      strcat( auxilary_name, ".aux" );
   }
   else                         /* The story file looks like: "OdieusQuest" */
   {
      strcat( save_name, ".sav" );
      strcat( script_name, ".src" );
      strcat( record_name, ".rec" );
      strcat( auxilary_name, ".aux" );
   }
}                               /* set_names */

/*
 * open_story
 *
 * Open game file for read.
 *
 */

void open_story( const char *storyname )
{
   char *path, *p;
   char tmp[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1];

   if ( !STANDALONE_FLAG )
   {
      story_offset = 0;
   }
   else
   {
      /* standalone game; offset to story start is saved in low-endian */
      /* format after magic string */
      story_offset =
            magic[MAGIC_END + 1] + magic[MAGIC_END + 2] * 256L + magic[MAGIC_END + 3] * 65536L;
   }

   strcpy( tmp, storyname );    
   if ( ( gfp = jz_open( tmp, "rb" ) ) != NULL )
   {                            
#if defined BUFFER_FILES        
#ifndef USE_ZLIB
      setbuf( gfp, gfpbuffer ); 
#endif
#endif 
      set_names( storyname );   
      return;                   
#if defined MSDOS || defined OS2 
   }                            
   else                         
   {                            
      sprintf( tmp, "%s.exe", storyname ); 
      if ( ( gfp = jz_open( tmp, "rb" ) ) != NULL )
      {                         
#if defined BUFFER_FILES        
#ifndef USE_ZLIB
         setbuf( gfp, gfpbuffer ); 
#endif
#endif 
         set_names( storyname ); 
         return;                
      }                         
#endif 
   }                            

   if ( !STANDALONE_FLAG && ( path = getenv( "INFOCOM_PATH" ) ) == NULL )
   {
      fprintf( stderr, "%s ", tmp );
      fatal( "open_story(): Zcode file not found" );
   }
   else if ( STANDALONE_FLAG && ( path = getenv( "PATH" ) ) == NULL )
   {
      fprintf( stderr, "%s ", tmp );
      fatal( "open_story(): Zcode file not found" );
   }

   /* dos path will be like:                                                 */
   /* SET INFOCOM_PATH = C:\INFOCOM\LTOI1;C:\INFOCOM\LTOI2;C:\INFOCOM\INFORM */
#if defined MSDOS || defined OS2 
   p = strtok( path, ";" );
#else
   /* UNIX path will be like:                                                */
   /* setenv INFOCOM_PATH /usr/local/lib/ltoi1:/usr/local/lib/ltoi2          */
   p = strtok( path, ":" );
#endif

   while ( p )
   {
      sprintf( tmp, "%s/%s", p, storyname );
      if ( ( gfp = jz_open( tmp, "rb" ) ) != NULL )
      {
#if defined BUFFER_FILES        
#ifndef USE_ZLIB
         setbuf( gfp, gfpbuffer ); 
#endif
#endif 
         set_names( storyname );
         return;
#if defined MSDOS || defined OS2 
      }                         
      else                      
      {                         
         sprintf( tmp, "%s/%s.exe", p, storyname ); 
         if ( ( gfp = jz_open( tmp, "rb" ) ) != NULL )
         {                      
#if defined BUFFER_FILES        
#ifndef USE_ZLIB
            setbuf( gfp, gfpbuffer ); 
#endif
#endif 
            set_names( storyname ); 
            return;             
         }                      
#endif 
      }
#if defined MSDOS || defined OS2 
      p = strtok( NULL, ";" );
#else
      p = strtok( NULL, ":" );
#endif
   }

   fprintf( stderr, "%s ", tmp );
   fatal( "open_story(): Zcode file not found" );
}                               /* open_story */


/*
 * close_story
 *
 * Close game file if open.
 *
 */

void close_story( void )
{

   if ( gfp != NULL )
   {
      jz_close( gfp );
   }

}                               /* close_story */

/*
 * get_story_size
 *
 * Calculate the size of the game file. Only used for very old games that do not
 * have the game file size in the header.
 *
 */

unsigned int get_story_size( void )
{
   unsigned long file_length;

   /* Read whole file to calculate file size */
   jz_rewind( gfp );
   for ( file_length = 0; jz_getc( gfp ) != EOF; file_length++ )
      ;
   jz_rewind( gfp );

   /* Calculate length of file in game allocation units */
   file_length =
         ( file_length + ( unsigned long ) ( story_scaler - 1 ) ) / ( unsigned long ) story_scaler;

   return ( ( unsigned int ) file_length );

}                               /* get_story_size */

/*
 * read_page
 *
 * Read one game file page.
 *
 */

void read_page( int page, void *buffer )
{
   unsigned long file_size;
   unsigned int pages, offset;

   /* Seek to start of page */
   jz_seek( gfp, story_offset + ( long ) page * PAGE_SIZE, SEEK_SET );

   /* Read the page */

#ifdef USE_ZLIB
   if ( gzread( gfp, buffer, PAGE_SIZE ) == -1 )
#else
   if ( fread( buffer, PAGE_SIZE, 1, gfp ) != 1 )
#endif
   {
      /* Read failed. Are we in the last page? */
      file_size = ( unsigned long ) h_file_size *story_scaler;

      pages = ( unsigned int ) ( ( unsigned long ) file_size / PAGE_SIZE );
      offset = ( unsigned int ) ( ( unsigned long ) file_size & PAGE_MASK );

      if ( ( unsigned int ) page == pages )
      {
         /* Read partial page if this is the last page in the game file */
         jz_seek( gfp, story_offset + ( long ) page * PAGE_SIZE, SEEK_SET );
#ifdef USE_ZLIB
         if ( gzread( gfp, buffer, offset ) == -1 )
#else
         if ( fread( buffer, offset, 1, gfp ) != 1 )
#endif
         {
            fatal( "read_page(): Zcode file read error" );
         }
      }
   }

}                               /* read_page */

/*
 * z_verify
 *
 * Verify game ($verify verb). Add all bytes in game file except for bytes in
 * the game file header.
 *
 */

void z_verify( void )
{
   unsigned long file_size;
   unsigned int pages, offset;
   unsigned int start, end, i, j;
   zword_t checksum = 0;
   zbyte_t buffer[PAGE_SIZE] = { 0 };
   char szBuffer[8] = { 0 };

   /* Print version banner */

   z_new_line(  );
   write_string( "Running on " );
   write_string( JZIPVER );
   write_string( " (" );
   switch ( JTERP )
   {
      case INTERP_GENERIC:
         write_string( "Generic" );
         break;
      case INTERP_AMIGA:
         write_string( "Amiga" );
         break;
      case INTERP_ATARI_ST:
         write_string( "Atari ST" );
         break;
      case INTERP_MSDOS:
         write_string( "DOS" );
         break;
      case INTERP_UNIX:
         write_string( "UNIX" );
         break;
      case INTERP_VMS:
         write_string( "VMS" );
         break;
   }
   write_string( "). Reporting Spec " );
   sprintf( szBuffer, "%d.%d", get_byte( H_STANDARD_HIGH ), get_byte( H_STANDARD_LOW ) );
   write_string( szBuffer );
   write_string( " Compliance." );
   z_new_line(  );

   write_string( "Compile options: " );
#ifdef USE_QUETZAL
   write_string( "USE_QUETZAL " );
#endif
#ifdef STRICTZ
   write_string( "STRICTZ " );
#endif
#ifdef USE_ZLIB
   write_string( "USE_ZLIB " );
#endif
#ifdef LOUSY_RANDOM
   write_string( "LOUSY_RANDOM " );
#endif
#ifdef HARD_COLORS
   write_string( "HARD_COLORS " );
#endif
   z_new_line(  );

   write_string( "Release " );
   write_string( JZIPRELDATE );
   write_string( "." );
   z_new_line(  );

   write_string( "Playing a Version " );
   z_print_num( (zword_t) GLOBALVER );
   write_string( " Story." );
   z_new_line(  );

   z_new_line(  );

   /* Calculate game file dimensions */

   file_size = ( unsigned long ) h_file_size *story_scaler;

   pages = ( unsigned int ) ( ( unsigned long ) file_size / PAGE_SIZE );
   offset = ( unsigned int ) file_size & PAGE_MASK;

   /* Sum all bytes in game file, except header bytes */

   for ( i = 0; i <= pages; i++ )
   {
      read_page( i, buffer );
      start = ( i == 0 ) ? 64 : 0;
      end = ( i == pages ) ? offset : PAGE_SIZE;
      for ( j = start; j < end; j++ )
      {
         checksum += buffer[j];
      }
   }

   /* Make a conditional jump based on whether the checksum is equal */

   conditional_jump( checksum == h_checksum );

}                               /* z_verify */


/*
 * get_default_name
 *
 * Read a default file name from the memory of the Z-machine and
 * copy it to an array.
 *
 */

static void get_default_name( char *default_name, zword_t addr )
{
   zbyte_t len;
   zbyte_t c;
   unsigned int i;

   if ( addr != 0 )
   {
      len = get_byte( addr );

      for ( i = 0; i < len; i++ )
      {
         addr++;
         c = get_byte( addr );
         default_name[i] = c;
      }
      default_name[i] = 0;

      if ( strchr( default_name, '.' ) == 0 )
      {
         strcpy( default_name + i, ".aux" );
      }

   }
   else
   {
      strcpy( default_name, auxilary_name );
   }

}                               /* get_default_name */


/*
 * z_save
 *
 * Saves data to disk. Returns:
 *     0 = save failed
 *     1 = save succeeded
 *
 */

int z_save( int argc, zword_t table, zword_t bytes, zword_t name )
{
   char new_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1];
   char default_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1];
   FILE *afp;

#if defined BUFFER_FILES        
   char afpbuffer[BUFSIZ];      
#endif 
   int status = 0;

   if ( argc == 3 )
   {
      get_default_name( default_name, name );
      if ( get_file_name( new_name, default_name, GAME_SAVE_AUX ) != 0 )
      {
         goto finished;
      }

      if ( ( afp = fopen( new_name, "wb" ) ) == NULL )
      {
         goto finished;
      }

#if defined BUFFER_FILES        
      setbuf( afp, afpbuffer ); 
#endif 

      status = fwrite( datap + table, bytes, 1, afp );

      fclose( afp );

      if ( status != 0 )
      {
         strcpy( auxilary_name, default_name );
      }

      status = !status;
   }
   else
   {
      /* Get the file name */
      status = 1;

      if ( get_file_name( new_name, save_name, GAME_SAVE ) == 0 )
      {
         /* Do a save operation */
         if ( save_restore( new_name, GAME_SAVE ) == 0 )
         {
            /* Cleanup file */
            file_cleanup( new_name, GAME_SAVE );

            /* Save the new name as the default file name */
            strcpy( save_name, new_name );

            /* Indicate success */
            status = 0;
         }
      }
   }

 finished:

   /* Return result of save to Z-code */

   if ( h_type < V4 )
   {
      conditional_jump( status == 0 );
   }
   else
   {
      store_operand( (zword_t)(( status == 0 ) ? 1 : 0) );
   }

   return ( status );
}                               /* z_save */


/*
 * z_restore
 *
 * Restore game state from disk. Returns:
 *     0 = restore failed
 *     2 = restore succeeded
 */

int z_restore( int argc, zword_t table, zword_t bytes, zword_t name )
{
   char new_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1];
   char default_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1];
   FILE *afp;

#if defined BUFFER_FILES        
   char afpbuffer[BUFSIZ];      
#endif 
   int status;

   status = 0;

   if ( argc == 3 )
   {
      get_default_name( default_name, name );
      if ( get_file_name( new_name, default_name, GAME_LOAD_AUX ) == 0 )
      {
         goto finished;
      }

      if ( ( afp = fopen( new_name, "rb" ) ) == NULL )
      {
         goto finished;
      }

#if defined BUFFER_FILES        
      setbuf( afp, afpbuffer ); 
#endif 

      status = fread( datap + table, bytes, 1, afp );

      fclose( afp );

      if ( status != 0 )
      {
         strcpy( auxilary_name, default_name );
      }

      status = !status;
   }
   else
   {
      /* Get the file name */
      status = 1;
      if ( get_file_name( new_name, save_name, GAME_RESTORE ) == 0 )
      {
         /* Do the restore operation */
         if ( save_restore( new_name, GAME_RESTORE ) == 0 )
         {
            /* Reset the status region (this is just for Seastalker) */
            if ( h_type < V4 )
            {
               z_split_window( 0 );
               blank_status_line(  );
            }

            /* Cleanup file */
            file_cleanup( new_name, GAME_SAVE );

            /* Save the new name as the default file name */
            strcpy( save_name, new_name );

            /* Indicate success */
            status = 0;
         }
      }
   }

 finished:
   /* Return result of save to Z-code */

   if ( h_type < V4 )
   {
      conditional_jump( status == 0 );
   }
   else
   {
      store_operand( (zword_t)(( status == 0 ) ? 2 : 0) );
   }

   return ( status );
}                               /* z_restore */

/*
 * z_save_undo
 *
 * Save the current Z machine state in memory for a future undo. Returns:
 *    -1 = feature unavailable
 *     0 = save failed
 *     1 = save succeeded
 *
 */

void z_save_undo( void )
{
   /* Check if undo is available first */
   if ( undo_datap != NULL )
   {
      /* Save the undo data and return success */
      save_restore( NULL, UNDO_SAVE );
      undo_valid = TRUE;
      store_operand( 1 );
   }
   else
   {
      /* If no memory for data area then say undo is not available */
      store_operand( ( zword_t ) - 1 );
   }

}                               /* z_save_undo */

/*
 * z_restore_undo
 *
 * Restore the current Z machine state from memory. Returns:
 *    -1 = feature unavailable
 *     0 = restore failed
 *     2 = restore succeeded
 *
 */

void z_restore_undo( void )
{
   /* Check if undo is available first */
   if ( undo_datap != NULL )
   {
      /* If no undo save done then return an error */
      if ( undo_valid == TRUE )
      {
         /* Restore the undo data and return success */
         save_restore( NULL, UNDO_RESTORE );
         store_operand( 2 );
      }
      else
      {
         store_operand( 0 );
      }
   }
   else
   {
      /* If no memory for data area then say undo is not available */
      store_operand( ( zword_t ) - 1 );
   }

}                               /* z_restore_undo */


/*
 * swap_bytes
 *
 * Swap the low and high bytes in every word of the specified array.
 * The length is specified in BYTES!
 *
 * This routine added by Mark Phillips(msp@bnr.co.uk), Thanks Mark!
 */
#if !defined(USE_QUETZAL)
void swap_bytes( zword_t * ptr, int len )
{
   unsigned char *pbyte;
   unsigned char tmp;

   len /= 2;                    /* convert len into number of 2 byte words */

   pbyte = ( unsigned char * ) ptr;

   while ( len )
   {
      tmp = pbyte[0];
      pbyte[0] = pbyte[1];
      pbyte[1] = tmp;
      pbyte += 2;
      len--;
   }
   return;
}
#endif

/*
 * save_restore
 *
 * Common save and restore code. Just save or restore the game stack and the
 * writeable data area.
 *
 */

static int save_restore( const char *file_name, int flag )
{
   FILE *tfp = NULL;

#if defined BUFFER_FILES        
   char tfpbuffer[BUFSIZ];      
#endif 
   int scripting_flag = 0, status = 0;

#if !defined(USE_QUETZAL)
   zword_t zw;
   int little_endian = 0;

   /* Find out if we are big-endian */
   zw = 0x0001;
   if ( *( zbyte_t * ) & zw )
   {                            /* We are little-endian, like an Intel 80x86 chip. */
      little_endian = 1;
   }
#endif

   /* Open the save file and disable scripting */

   if ( flag == GAME_SAVE || flag == GAME_RESTORE )
   {
      if ( ( tfp = fopen( file_name, ( flag == GAME_SAVE ) ? "wb" : "rb" ) ) == NULL )
      {
         output_line( "Cannot open SAVE file" );
         return ( 1 );
      }
#if defined BUFFER_FILES        
      setbuf( tfp, tfpbuffer ); 
#endif 
      scripting_flag = get_word( H_FLAGS ) & SCRIPTING_FLAG;
      set_word( H_FLAGS, get_word( H_FLAGS ) & ( ~SCRIPTING_FLAG ) );
   }

#if defined(USE_QUETZAL)
   if ( flag == GAME_SAVE )
   {
      status = !save_quetzal( tfp, gfp );
   }
   else if ( flag == GAME_RESTORE )
   {
      status = !restore_quetzal( tfp, gfp );
   }
   else
   {
#endif /* defined(USE_QUETZAL) */
      /* Push PC, FP, version and store SP in special location */

      stack[--sp] = ( zword_t ) ( pc / PAGE_SIZE );
      stack[--sp] = ( zword_t ) ( pc % PAGE_SIZE );
      stack[--sp] = fp;
      stack[--sp] = h_version;
      stack[0] = sp;

      /* Save or restore stack */

#if !defined(USE_QUETZAL)
      if ( flag == GAME_SAVE )
      {
         if ( little_endian )
            swap_bytes( stack, sizeof ( stack ) );
         if ( status == 0 && fwrite( stack, sizeof ( stack ), 1, tfp ) != 1 )
            status = 1;
         if ( little_endian )
            swap_bytes( stack, sizeof ( stack ) );
      }
      else if ( flag == GAME_RESTORE )
      {
         if ( little_endian )
            swap_bytes( stack, sizeof ( stack ) );
         if ( status == 0 && fread( stack, sizeof ( stack ), 1, tfp ) != 1 )
            status = 1;
         if ( little_endian )
            swap_bytes( stack, sizeof ( stack ) );
      }
      else
#endif /* !defined(USE_QUETZAL) */
      {
         if ( flag == UNDO_SAVE )
         {
            memmove( undo_stack, stack, sizeof ( stack ) );
         }
         else                   /* if (flag == UNDO_RESTORE) */
         {
            memmove( stack, undo_stack, sizeof ( stack ) );
         }
      }

      /* Restore SP, check version, restore FP and PC */

      sp = stack[0];

      if ( stack[sp++] != h_version )
      {
         fatal( "save_restore(): Wrong game or version" );
      }

      fp = stack[sp++];
      pc = stack[sp++];
      pc += ( unsigned long ) stack[sp++] * PAGE_SIZE;

      /* Save or restore writeable game data area */

#if !defined(USE_QUETZAL)
      if ( flag == GAME_SAVE )
      {
         if ( status == 0 && fwrite( datap, h_restart_size, 1, tfp ) != 1 )
            status = 1;
      }
      else if ( flag == GAME_RESTORE )
      {
         if ( status == 0 && fread( datap, h_restart_size, 1, tfp ) != 1 )
            status = 1;
      }
      else
#endif /* !defined(USE_QUETZAL) */
      {
         if ( flag == UNDO_SAVE )
         {
            memmove( undo_datap, datap, h_restart_size );
         }
         else                   /* if (flag == UNDO_RESTORE) */
         {
            memmove( datap, undo_datap, h_restart_size );
         }
      }

#if defined(USE_QUETZAL)
   }
#endif /* defined(USE_QUETZAL) */


   /* Close the save file and restore scripting */

   if ( flag == GAME_SAVE )
   {
      fclose( tfp );
      if ( scripting_flag )
      {
         set_word( H_FLAGS, get_word( H_FLAGS ) | SCRIPTING_FLAG );
      }
   }
   else if ( flag == GAME_RESTORE )
   {
      fclose( tfp );
      restart_screen(  );
      restart_interp( scripting_flag );
   }

   /* Handle read or write errors */

   if ( status )
   {
      if ( flag == GAME_SAVE )
      {
         output_line( "Write to SAVE file failed" );
         remove( file_name );
      }
      else
      {
         fatal( "save_restore(): Read from SAVE file failed" );
      }
   }

   return ( status );

}                               /* save_restore */

/*
 * open_script
 *
 * Open the scripting file.
 *
 */

void open_script( void )
{
   char new_script_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1];

   /* Open scripting file if closed */
   if ( scripting == OFF )
   {
      if ( script_file_valid == TRUE )
      {
         sfp = fopen( script_name, "a" );

         /* Turn on scripting if open succeeded */
         if ( sfp != NULL )
         {
#if defined BUFFER_FILES        
            setbuf( sfp, sfpbuffer ); 
#endif 
            scripting = ON;
         }
         else
         {
            output_line( "Script file open failed" );
         }
      }
      else
      {                         /* Get scripting file name and record it */
         if ( get_file_name( new_script_name, script_name, GAME_SCRIPT ) == 0 )
         {
            /* Open scripting file */
            sfp = fopen( new_script_name, "w" );

            /* Turn on scripting if open succeeded */
            if ( sfp != NULL )
            {
#if defined BUFFER_FILES        
               setbuf( sfp, sfpbuffer ); 
#endif 
               script_file_valid = TRUE;

               /* Make file name the default name */
               strcpy( script_name, new_script_name );

               /* Turn on scripting */
               scripting = ON;
            }
            else
            {
               output_line( "Script file create failed" );
            }
         }
      }
   }

   /* Set the scripting flag in the game file flags */
   if ( scripting == ON )
   {
      set_word( H_FLAGS, get_word( H_FLAGS ) | SCRIPTING_FLAG );
   }
   else
   {
      set_word( H_FLAGS, get_word( H_FLAGS ) & ( ~SCRIPTING_FLAG ) );
   }

}                               /* open_script */

/*
 * flush_script
 * Flush the scripting file.
 *
 */
void flush_script( void )       
{                               
/* Flush scripting file if open */
   if ( scripting == ON )       
   {                            
      fflush( sfp );            
   }                            
}                               


/*
 * close_script
 *
 * Close the scripting file.
 *
 */
void close_script( void )
{
   /* Close scripting file if open */
   if ( scripting == ON )
   {
      fclose( sfp );
#if 0
      /* Cleanup */
      file_cleanup( script_name, GAME_SCRIPT );
#endif
      /* Turn off scripting */
      scripting = OFF;
   }

   /* Set the scripting flag in the game file flags */
   if ( scripting == OFF )
   {
      set_word( H_FLAGS, get_word( H_FLAGS ) & ( ~SCRIPTING_FLAG ) );
   }
   else
   {
      set_word( H_FLAGS, get_word( H_FLAGS ) | SCRIPTING_FLAG );
   }

}                               /* close_script */

/*
 * script_char
 *
 * Write one character to scripting file.
 *
 * Check the state of the scripting flag first. Older games only set the
 * scripting flag in the game flags instead of calling the set_print_modes
 * function. This is because they expect a physically attached printer that
 * doesn't need opening like a file.
 */

void script_char( int c )
{

   /* Check the state of the scripting flag in the game flags. If it is on
    * then check to see if the scripting file is open as well */

   if ( ( get_word( H_FLAGS ) & SCRIPTING_FLAG ) != 0 && scripting == OFF )
   {
      open_script(  );
   }

   /* Check the state of the scripting flag in the game flags. If it is off
    * then check to see if the scripting file is closed as well */

   if ( ( get_word( H_FLAGS ) & SCRIPTING_FLAG ) == 0 && scripting == ON )
   {
      close_script(  );
   }

   /* If scripting file is open, we are in the text window and the character is
    * printable then write the character */

   if ( scripting == ON && scripting_disable == OFF && ( c == '\n' || ( isprint( c ) ) ) )
   {
      putc( c, sfp );
   }

}                               /* script_char */

/*
 * script_string
 *
 * Write a string to the scripting file.
 *
 */

void script_string( const char *s )
{
   /* Write string */
   while ( *s )
   {
      script_char( *s++ );
   }

}                               /* script_string */

/*
 * script_line
 *
 * Write a string followed by a new line to the scripting file.
 *
 */

void script_line( const char *s )
{

   /* Write string */
   script_string( s );

   /* Write new line */
   script_new_line(  );

}                               /* script_line */

/*
 * script_new_line
 *
 * Write a new line to the scripting file.
 *
 */

void script_new_line( void )
{

   script_char( '\n' );

}                               /* script_new_line */

/*
 * open_record
 *
 * Turn on recording of all input to an output file.
 *
 */

void open_record( void )
{
   char new_record_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1];

   /* If recording or playback is already on then complain */

   if ( recording == ON || replaying == ON )
   {
      output_line( "Recording or playback are already active." );
   }
   else
   {                            /* Get recording file name */
      if ( get_file_name( new_record_name, record_name, GAME_RECORD ) == 0 )
      {
         /* Open recording file */
         rfp = fopen( new_record_name, "w" );

         /* Turn on recording if open succeeded */
         if ( rfp != NULL )
         {
#if defined BUFFER_FILES        
            setbuf( rfp, rfpbuffer ); 
#endif 
            /* Make file name the default name */
            strcpy( record_name, new_record_name );

            /* Set recording on */
            recording = ON;
         }
         else
         {
            output_line( "Record file create failed" );
         }
      }
   }

}                               /* open_record */

/*
 * record_line
 *
 * Write a string followed by a new line to the recording file.
 *
 */

void record_line( const char *s )
{
   if ( recording == ON && replaying == OFF )
   {
      /* Write string */
      fprintf( rfp, "%s\n", s );
   }

}                               /* record_line */

/*
 * record_key
 *
 * Write a key followed by a new line to the recording file.
 *
 */

void record_key( int c )
{
   if ( recording == ON && replaying == OFF )
   {
      /* Write the key */
      fprintf( rfp, "<%0o>\n", c );
   }

}                               /* record_key */

/*
 * close_record
 *
 * Turn off recording of all input to an output file.
 *
 */

void close_record( void )
{
   /* Close recording file */
   if ( rfp != NULL )
   {
      fclose( rfp );
      rfp = NULL;

      /* Cleanup */

      if ( recording == ON )
      {
         file_cleanup( record_name, GAME_RECORD );
      }
      else                      /* (replaying == ON) */
      {
         file_cleanup( record_name, GAME_PLAYBACK );
      }
   }

   /* Set recording and replaying off */

   recording = OFF;
   replaying = OFF;

}                               /* close_record */

/*
 * z_input_stream
 *
 * Take input from command file instead of keyboard.
 *
 */

void z_input_stream( int arg )
{
   char new_record_name[Z_FILENAME_MAX + Z_PATHNAME_MAX + 1];

   UNUSEDVAR( arg );

   /* If recording or replaying is already on then complain */

   if ( recording == ON || replaying == ON )
   {
      output_line( "Recording or replaying is already active." );
   }
   else
   {                            /* Get recording file name */

      if ( get_file_name( new_record_name, record_name, GAME_PLAYBACK ) == 0 )
      {
         /* Open recording file */
         rfp = fopen( new_record_name, "r" );

         /* Turn on recording if open succeeded */
         if ( rfp != NULL )
         {
#if defined BUFFER_FILES        
            setbuf( rfp, rfpbuffer ); 
#endif 
            /* Make file name the default name */
            strcpy( record_name, new_record_name );

            /* Set replaying on */
            replaying = ON;
         }
         else
         {
            output_line( "Record file open failed" );
         }
      }
   }

}                               /* z_input_stream */

/*
 * playback_line
 *
 * Get a line of input from the command file.
 *
 */

int playback_line( int buflen, char *buffer, int *read_size )
{
   char *cp;

   if ( recording == ON || replaying == OFF )
   {
      return ( -1 );
   }

   if ( fgets( buffer, buflen, rfp ) == NULL )
   {
      close_record(  );
      return ( -1 );
   }
   else
   {
      cp = strrchr( buffer, '\n' );
      if ( cp != NULL )
      {
         *cp = '\0';
      }
      *read_size = strlen( buffer );
      output_line( buffer );
   }

   return ( '\n' );

}                               /* playback_line */

/*
 * playback_key
 *
 * Get a key from the command file.
 *
 */

int playback_key( void )
{
   int c;

   if ( recording == ON || replaying == OFF )
   {
      return ( -1 );
   }

   if ( fscanf( rfp, "<%o>\n", &c ) == EOF )
   {
      close_record(  );
      c = -1;
   }

   return ( c );

}                               /* playback_key */
