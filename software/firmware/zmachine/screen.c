
/* $Id: screen.c,v 1.3 2000/07/05 15:20:34 jholder Exp $   
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information   
 * --------------------------------------------------------------------
 * 
 * File name: $Id: screen.c,v 1.3 2000/07/05 15:20:34 jholder Exp $  
 *   
 * Description:    
 *    
 * Modification history:      
 * $Log: screen.c,v $
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
 * screen.c
 *
 * Generic screen manipulation routines. Most of these routines call the machine
 * specific routines to do the actual work.
 *
 */

#include "ztypes.h"

/*
 * z_set_window
 *
 * Put the cursor in the text or status window. The cursor is free to move in
 * the status window, but is fixed to the input line in the text window.
 *
 */

void z_set_window( zword_t w )
{
   int row, col;

   flush_buffer( FALSE );

   screen_window = w;

   if ( screen_window == STATUS_WINDOW )
   {

      /* Status window: disable formatting and select status window */

      formatting = OFF;
      scripting_disable = ON;
      select_status_window(  );

      /* Put cursor at top of status area */

      if ( h_type < V4 )
         move_cursor( 2, 1 );
      else
         move_cursor( 1, 1 );

   }
   else
   {

      /* Text window: enable formatting and select text window */

      select_text_window(  );
      scripting_disable = OFF;
      formatting = ON;

      /* Move cursor if it has been left in the status area */

      get_cursor_position( &row, &col );
      if ( row <= status_size )
         move_cursor( status_size + 1, 1 );

   }

   /* Force text attribute to normal rendition */

   set_attribute( NORMAL );

}                               /* z_set_window */

/*
 * z_split_window
 *
 * Set the size of the status window. The default size for the status window is
 * zero lines for both type 3 and 4 games. The status line is handled specially
 * for type 3 games and always occurs the line immediately above the status
 * window.
 *
 */

void z_split_window( zword_t lines )
{
   /* Maximum status window size is 255 */

   lines &= 0xff;

   /* The top line is always set for V1 to V3 games, so account for it here. */

   if ( h_type < V4 )
      lines++;

   if ( lines )
   {

      /* If size is non zero the turn on the status window */

      status_active = ON;

      /* Bound the status size to one line less than the total screen height */

      if ( lines > ( zword_t ) ( screen_rows - 1 ) )
         status_size = ( zword_t ) ( screen_rows - 1 );
      else
         status_size = lines;

      /* Create the status window, or resize it */

      create_status_window(  );

      /* Need to clear the status window for type 3 games */

      if ( h_type < V4 )
         z_erase_window( STATUS_WINDOW );

   }
   else
   {

      /* Lines are zero so turn off the status window */

      status_active = OFF;

      /* Reset the lines written counter and status size */

      lines_written = 0;
      status_size = 0;

      /* Delete the status window */

      delete_status_window(  );

      /* Return cursor to text window */

      select_text_window(  );
   }

}                               /* z_split_window */

/*
 * z_erase_window
 *
 * Clear one or all windows on the screen.
 *
 */

void z_erase_window( zword_t w )
{
   flush_buffer( TRUE );

   if ( ( zbyte_t ) w == ( zbyte_t ) Z_SCREEN )
   {
      clear_screen(  );
   }
   else if ( ( zbyte_t ) w == TEXT_WINDOW )
   {
      clear_text_window(  );
   }
   else if ( ( zbyte_t ) w == STATUS_WINDOW )
   {
      clear_status_window(  );
      return;
   }

   if ( h_type > V4 )
      move_cursor( 1, 1 );
   else
      move_cursor( screen_rows, 1 );

}                               /* z_erase_window */

/*
 * z_erase_line
 *
 * Clear one line on the screen.
 *
 */

void z_erase_line( zword_t flag )
{
   if ( flag == TRUE )
      clear_line(  );
}                               /* z_erase_line */

/*
 * z_set_cursor
 *
 * Set the cursor position in the status window only.
 *
 */

void z_set_cursor( zword_t row, zword_t column )
{
   /* Can only move cursor if format mode is off and in status window */

   if ( formatting == OFF && screen_window == STATUS_WINDOW )
   {
      move_cursor( row, column );
   }
#ifdef STRICTZ
   else
   {
      report_strictz_error( STRZERR_MOV_CURSOR, "@set_cursor called outside the status window!" );
   }
#endif
}                               /* z_set_cursor */

/*
 * pad_line
 *
 * Pad the status line with spaces up to a column position.
 *
 */

static void pad_line( int column )
{
   int i;

   for ( i = status_pos; i < column; i++ )
      write_char( ' ' );
   status_pos = column;
}                               /* pad_line */

/*
 * z_show_status
 *
 * Format and output the status line for type 3 games only.
 *
 */

void z_show_status( void )
{
   int i, count = 0, end_of_string[3];
   char *status_part[3];

   /* Move the cursor to the top line of the status window, set the reverse
    * rendition and print the status line */

   z_set_window( STATUS_WINDOW );
   move_cursor( 1, 1 );
   set_attribute( REVERSE );

   /* Redirect output to the status line buffer */

   z_output_stream( 3, 0 );

   /* Print the object description for global variable 16 */

   pad_line( 1 );
   status_part[count] = &status_line[status_pos];
   if ( load_variable( 16 ) != 0 )
      z_print_obj( load_variable( 16 ) );
   end_of_string[count++] = status_pos;
   status_line[status_pos++] = '\0';

   if ( get_byte( H_CONFIG ) & CONFIG_TIME )
   {

      /* If a time display print the hours and minutes from global
       * variables 17 and 18 */

      pad_line( screen_cols - 21 );
      status_part[count] = &status_line[status_pos];
      write_string( " Time: " );
      print_time( load_variable( 17 ), load_variable( 18 ) );
      end_of_string[count++] = status_pos;
      status_line[status_pos++] = '\0';
   }
   else
   {

      /* If a moves/score display print the score and moves from global
       * variables 17 and 18 */

      pad_line( screen_cols - 31 );
      status_part[count] = &status_line[status_pos];
      write_string( " Score: " );
      z_print_num( load_variable( 17 ) );
      end_of_string[count++] = status_pos;
      status_line[status_pos++] = '\0';

      pad_line( screen_cols - 15 );
      status_part[count] = &status_line[status_pos];
      write_string( " Moves: " );
      z_print_num( load_variable( 18 ) );
      end_of_string[count++] = status_pos;
      status_line[status_pos++] = '\0';
   }

   /* Pad the end of status line with spaces then disable output redirection */

   pad_line( screen_cols );
   z_output_stream( ( zword_t ) - 3, 0 );

   /* Try and print the status line for a proportional font screen. If this
    * fails then remove embedded nulls in status line buffer and just output
    * it to the screen */

   if ( print_status( count, status_part ) == FALSE )
   {
      for ( i = 0; i < count; i++ )
         status_line[end_of_string[i]] = ' ';
      status_line[status_pos] = '\0';
      write_string( status_line );
   }

   set_attribute( NORMAL );
   z_set_window( TEXT_WINDOW );

}                               /* z_show_status */

/*
 * blank_status_line
 *
 * Output a blank status line for type 3 games only.
 *
 */

void blank_status_line( void )
{

   /* Move the cursor to the top line of the status window, set the reverse
    * rendition and print the status line */

   z_set_window( STATUS_WINDOW );
   move_cursor( 1, 1 );
   set_attribute( REVERSE );

   /* Redirect output to the status line buffer and pad the status line with
    * spaces then disable output redirection */

   z_output_stream( 3, 0 );
   pad_line( screen_cols );
   status_line[status_pos] = '\0';
   z_output_stream( ( zword_t ) - 3, 0 );

   /* Write the status line */

   write_string( status_line );

   /* Turn off attributes and return to text window */

   set_attribute( NORMAL );
   z_set_window( TEXT_WINDOW );

}                               /* blank_status_line */

/*
 * output_string
 *
 * Output a string of characters.
 *
 */

void output_string( const char *s )
{
   while ( *s )
      output_char( *s++ );
}                               /* output_string */

/*
 * output_line
 *
 * Output a string of characters followed by a new line.
 *
 */

void output_line( const char *s )
{
   output_string( s );
   output_new_line(  );
}                               /* output_line */

/*
 * output_char
 *
 * Output a character.
 *
 */

void output_char( int c )
{
   /* If output is enabled then either select the rendition attribute
    * or just display the character */

   if ( outputting == ON )
   {
      display_char( (unsigned int)(c & 0xff) );
   }
}                               /* output_char */

/*
 * output_new_line
 *
 * Scroll the text window up one line and pause the window if it is full.
 *
 */

void output_new_line( void )
{
   int row, col;

   /* Don't print if output is disabled or replaying commands */

   if ( outputting == ON )
   {

      if ( formatting == ON && screen_window == TEXT_WINDOW )
      {

         /* If this is the text window then scroll it up one line */

         scroll_line(  );

         /* See if we have filled the screen. The spare line is for 
          * the [MORE] message 
          */

         if ( ++lines_written >= ( ( screen_rows - top_margin ) - status_size - 1 ) )
         {

            /* Display the new status line while the screen in paused */

            if ( h_type < V4 )
               z_show_status(  );

            /* Reset the line count and display the more message */

            lines_written = 0;

            if ( replaying == OFF )
            {
               get_cursor_position( &row, &col );
               output_string( "[MORE]" );
               ( void ) input_character( 0 );
               move_cursor( row, col );
               output_string( "      " );
               move_cursor( row, col );
               /* clear_line (); */
            }
         }
      }
      else
         /* If this is the status window then just output a new line */

         output_char( '\n' );
   }

}                               /* output_new_line */

/*
 * z_print_table
 *
 * Writes text into a rectangular window on the screen.
 *
 *    argv[0] = start of text address
 *    argv[1] = rectangle width
 *    argv[2] = rectangle height (default = 1)
 *
 */

void z_print_table( int argc, zword_t * argv )
{
   unsigned long address;
   unsigned int width, height;
   unsigned int row, column;

   /* Supply default arguments */

   if ( argc < 3 )
      argv[2] = 1;

   /* Don't do anything if the window is zero high or wide */

   if ( argv[1] == 0 || argv[2] == 0 )
      return;

   /* Get coordinates of top left corner of rectangle */

   get_cursor_position( ( int * ) &row, ( int * ) &column );

   address = argv[0];

   /* Write text in width * height rectangle */

   for ( height = 0; height < argv[2]; height++ )
   {

      for ( width = 0; width < argv[1]; width++ )
         write_char( read_data_byte( &address ) );

      /* Put cursor back to lefthand side of rectangle on next line */

      if ( height != (unsigned)( argv[2] - 1 ) )
         move_cursor( ++row, column );

   }

}                               /* z_print_table */

/*
 * z_set_font
 *
 * Set text or graphic font. 1 = text font, 3 = graphics font.
 *
 */

void z_set_font( zword_t new_font )
{
   zword_t old_font = font;

   if ( new_font != old_font )
   {
      font = new_font;
      set_font( font );
   }

   store_operand( old_font );

}                               /* z_set_font */

/*
 * z_set_colour
 *
 * Set the colour of the screen. Colour can be set on four things:
 *    Screen background
 *    Text typed by player
 *    Text written by game
 *    Graphics characters
 *
 * Colors can be set to 1 of 9 values:
 *    1 = machine default (IBM/PC = blue background, everything else white)
 *    2 = black
 *    3 = red
 *    4 = green
 *    5 = brown
 *    6 = blue
 *    7 = magenta
 *    8 = cyan
 *    9 = white
 *
 */

void z_set_colour( zword_t foreground, zword_t background )
{
   if ( ( ZINT16 ) foreground < -1 || ( ZINT16 ) foreground > 9 || ( ZINT16 ) background < -1 ||
        ( ZINT16 ) background > 9 )
      fatal( "Bad colour!" );


   flush_buffer( FALSE );

   set_colours( foreground, background );

   return;
}                               /* z_set_colour */
