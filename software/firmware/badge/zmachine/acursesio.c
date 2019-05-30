
/* 
 * Arduino version of curses library
 */

#include "ztypes.h"

#include <mcurses.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>

ztheme_t themes[6] = {
  {"TRS-80 Black", (F_BLACK|B_WHITE), (F_WHITE|B_BLACK)},
  {"Lisa White", (F_WHITE|B_BLACK), (F_BLACK|B_WHITE)},
  {"Compaq Green", (F_BLACK|B_GREEN), (F_GREEN|B_BLACK)},
  {"IBM XT Amber", (F_BLACK|B_YELLOW|A_DIM), (F_YELLOW|B_BLACK|A_DIM)},
  {"Amiga Blue", (F_BLUE|B_WHITE), (F_WHITE|B_BLUE)},
  //{"Blue and Gold", (F_YELLOW|B_BLUE|A_BOLD), (F_YELLOW|B_BLACK|A_BOLD)}
};
int themecount = 5;

extern int theme;
#define EXTENDED 1
#define PLAIN    2


#ifdef HARD_COLORS
static ZINT16 current_fg;
static ZINT16 current_bg;
#endif
extern ZINT16 default_fg;
extern ZINT16 default_bg;

extern int hist_buf_size;
extern int use_bg_color;

/* new stuff for command editing */
int BUFFER_SIZE;
char *commands;
int space_avail;
static int ptr1, ptr2 = 0;
static int end_ptr = 0;
static int row, head_col;
static int keypad_avail = 1;

/* done with editing global info */

static int current_row = 0;
static int current_col = 0;

static int saved_row;
static int saved_col;

static int status_row = 0;
static int status_col = 0;
static int text_col = 0;

static int cursor_saved = OFF;

static char tcbuf[1024];
static char cmbuf[1024];
static char *cmbufp;

static ZINT16 current_fg;
static ZINT16 current_bg;
extern ZINT16 default_fg;
extern ZINT16 default_bg;

static void display_string( char * );
static int read_char( void );

void Arduino_putchar(uint8_t c)
{
  Serial.write(c);
}

char Arduino_getchar()
{
  char c;
  while (!Serial.available());
  return Serial.read();
}

static int inc( void )
{
   //int c = getchar(  );

   while(!Serial.available());
   int c = Serial.read();
   if ( c == -1 )
   {
      fatal("acursesio inc: error in Serial.read!");
   }
   if((int) c != 127) // is this a backspace key? will print it later
    printf((char) c);
   if(c == '\r')
    printf((char) '\n');
   return c;
}

static int uninc( int c )
{
    // not supported right now
   //return ungetc( c, stdin );
}

static int outc( int c )
{
   printf((char) c);
   return c;
}

void initialize_screen(  )
{
   int row, col;

   setFunction_putchar(Arduino_putchar); // tell the library which output channel shall be used
   setFunction_getchar(Arduino_getchar); // tell the library which input channel shall be used

   /* initialize the command buffer */
   cmbufp = cmbuf;

   /* start the curses environment */
   if ( !initscr(  ) )
   {
      fatal( "initialize_screen(): Couldn't init curses." );
   }

   /* COLS and LINES set by curses */
   screen_cols = COLS;
   screen_rows = LINES;
   attrset(themes[theme].text_attr);

   clear_screen(  );

   /* Last release (2.0.1g) claimed DEC tops 20.  I'm a sadist. Sue me. */
   h_interpreter = INTERP_MSDOS;
   JTERP = INTERP_UNIX;

// command history not implemented
/*
   commands = ( char * ) malloc( hist_buf_size * sizeof ( char ) );

   if ( commands == NULL )
      fatal( "initialize_screen(): Couldn't allocate history buffer." );
   BUFFER_SIZE = hist_buf_size;
   space_avail = hist_buf_size - 1;
*/
   interp_initialized = 1;

}                               /* initialize_screen */

void restart_screen(  )
{
   cursor_saved = OFF;
}                               /* restart_screen */

void reset_screen(  )
{
   /* only do this stuff on exit when called AFTER initialize_screen */
   if ( interp_initialized )
   {
      display_string( "\r\n[Hit any key to exit.]" );
      getch(  );

      delete_status_window(  );
      select_text_window(  );

      //printf( "[0m" );

      erase(  );
      //set_cbreak_mode( 0 );

   }
   display_string( "\r\n" );

}                               /* reset_screen */

void clear_screen(  )
{
   erase(  );                   /* clear screen */
   current_row = 1;
   current_col = 1;
}                               /* clear_screen */


void select_status_window(  )
{
   save_cursor_position(  );
}                               /* select_status_window */


void select_text_window(  )
{
   restore_cursor_position(  );
}                               /* select_text_window */

void create_status_window(  )
{
   int row, col;

   get_cursor_position( &row, &col );

   /* set up a software scrolling region */

/*
    setscrreg(status_size, screen_rows-1);
*/

   move_cursor( row, col );
}                               /* create_status_window */

void delete_status_window(  )
{
   int row, col;

   get_cursor_position( &row, &col );

   /* set up a software scrolling region */
   
   setscrreg(0, screen_rows-1);

   move_cursor( row, col );

}                               /* delete_status_window */

void clear_line(  )
{
   clrtoeol(  );
}                               /* clear_line */

void clear_text_window(  )
{
   int i, row, col;

   get_cursor_position( &row, &col );

   for ( i = status_size + 1; i <= screen_rows; i++ )
   {
      move_cursor( i, 1 );
      clear_line(  );
   }

   move_cursor( row, col );

}                               /* clear_text_window */

void clear_status_window(  )
{
   int i, row, col;

   get_cursor_position( &row, &col );

   for ( i = status_size; i; i-- )
   {
      move_cursor( i, 1 );
      clear_line(  );
   }

   move_cursor( row, col );

}                               /* clear_status_window */

void move_cursor( int row, int col )
{
   move( row - 1, col - 1 );
   current_row = row;
   current_col = col;

}                               /* move_cursor */

void get_cursor_position( int *row, int *col )
{
   *row = current_row;
   *col = current_col;
}                               /* get_cursor_position */

void save_cursor_position(  )
{
   if ( cursor_saved == OFF )
   {
      get_cursor_position( &saved_row, &saved_col );
      cursor_saved = ON;
   }
}                               /* save_cursor_position */


void restore_cursor_position(  )
{
   if ( cursor_saved == ON )
   {
      move_cursor( saved_row, saved_col );
      cursor_saved = OFF;
   }
}                               /* restore_cursor_position */

void set_attribute( int attribute )
{
   static int emph = 0, rev = 0;

   if ( attribute == NORMAL )
   {
      // this is the text part of the window
         attrset(themes[theme].text_attr);
   }

   if ( attribute & REVERSE )
   {
      // this is the status part of the window
      attrset(themes[theme].status_attr);
   }

   if ( attribute & BOLD )
   {
   }
   if ( attribute & EMPHASIS )
   {
   }

   if ( attribute & FIXED_FONT )
   {
   }

}                               /* set_attribute */

static void display_string( char *s )
{
   while ( *s )
      display_char( *s++ );
}                               /* display_string */

void display_char( int c )
{
   outc( c );
   if ( ++current_col > screen_cols )
      current_col = screen_cols;
}                               /* display_char */

void scroll_line(  )
{
   int row, col;
   display_char( '\r' );
   display_char( '\n');

   get_cursor_position( &row, &col );

   if ( row < screen_rows )
   {
     display_char( '\r' );
     display_char( '\n');
   }
   else
   {
      setscrreg( status_size, screen_rows - 1 );
      display_char( '\r' );
      //display_char( '\n');
   }

   current_col = 1;
   if ( ++current_row > screen_rows )
      current_row = screen_rows;

}                               /* scroll_line */

int input_line( int buflen, char *buffer, int timeout, int *read_size )
{
   int c;

   *read_size = 0;

   // while ( ( c = read_char(  ) ) != '\n' )
   while ( ( c = read_char(  ) ) != '\r' ) // use for Arduino line feed
   {
      if(c == 127) // backspace pressed?
      {
        if(*read_size > 0)
        {
          buffer[(*read_size)] = '\0';
          (*read_size)--;
          printf((char) c); // OK to backspace, characters still on the left side
        }
      }
      else if ( *read_size < buflen )
         buffer[( *read_size )++] = c;
   }
   text_col = 0;
   return c;
}                               /* input_line */

#define COMMAND_LEN 20
static int read_char( void )
{
   static int input_is_at_eol = TRUE;
   int c, n;
   char command[COMMAND_LEN + 1];

   for ( ;; )
   {
      if ( ( c = inc(  ) ) == '\\' )
      {
         c = inc(  );
         if ( c == '\\' )
            break;
         uninc( c );
         /* Read a command.  */
         for ( n = 0; n < COMMAND_LEN; n++ )
         {
            command[n] = inc(  );
            //if ( command[n] == '\n' )
            if ( command[n] == '\r' )
               break;
         }
         command[n] = '\0';
         /* If line was too long, flush input to the end of it.  */
         if ( n == COMMAND_LEN )
            //while ( inc(  ) != '\n' )
            while ( inc(  ) != '\r' )
               ;
         continue;
      }
      break;
   }
   input_is_at_eol = ( c == '\r' );
   return c;
}                               /* read_char */

int input_character( int timeout )
{
   int c = read_char(  );

   /* Bureaucracy expects CR, not NL.  */
   return ( ( c == '\n' ) ? '\r' : c );
}                               /* input_character */

static int read_key( int mode )
{
   int c;

   if ( mode == PLAIN )
   {
      do
      {
         c = getch(  );
         if ( c == 4 )
         {
            reset_screen(  );
            exit( 0 );
         }                      /* CTRL-D (EOF) */
      }
      while ( !( c == 10 || c == 13 || c == 8 ) && ( c < 32 || c > 127 ) );
   }
   else if ( mode == EXTENDED )
   {                            /* also pass ESC character back for editor */
      do
      {
         c = getch(  );
         if ( c == 4 )
         {
            reset_screen(  );
            exit( 0 );
         }                      /* CTRL-D (EOF) */
      }
      while ( !( c == 27 || c == 10 || c == 13 || c == 8 ) && ( c < 32 || c > 127 ) );
   }

   if ( c == 127 )
      c = '\b';
   else if ( c == 10 )
      c = 13;

   return ( c );

}                               /* read_key */

static void rundown(  )
{
   unload_cache(  );
   close_story(  );
   close_script(  );
   reset_screen(  );
}                               /* rundown */


void set_colours( zword_t foreground, zword_t background )
{
  // not implemented
}
/* Zcolors:
 * BLACK 0   BLUE 4   GREEN 2   CYAN 6   RED 1   MAGENTA 5   BROWN 3   WHITE 7
 * ANSI Colors (foreground over background):
 * BLACK 30  BLUE 34  GREEN 32  CYAN 36  RED 31  MAGENTA 35  BROWN 33  WHITE 37
 * BLACK 40  BLUE 44  GREEN 42  CYAN 46  RED 41  MAGENTA 45  BROWN 43  WHITE 47
 */
void set_colours_( zword_t foreground, zword_t background )
{
   return;
   int fg, bg;

   int fg_colour_map[] = { F_BLACK, F_BLUE, F_GREEN, F_CYAN, F_RED, F_MAGENTA, F_BROWN, F_WHITE };
   int bg_colour_map[] = { B_BLACK, B_BLUE, B_GREEN, B_CYAN, B_RED, B_MAGENTA, B_BROWN, B_WHITE };

   /* Translate from Z-code colour values to natural colour values */

   if ( ( ZINT16 ) foreground >= 1 && ( ZINT16 ) foreground <= 9 )
   {
      fg = ( foreground == 1 ) ? ( fg_colour_map[default_fg] ) : fg_colour_map[foreground];
   }
   if ( ( ZINT16 ) background >= 1 && ( ZINT16 ) background <= 9 )
   {
      bg = ( background == 1 ) ? ( bg_colour_map[default_bg] ) : bg_colour_map[background];
   }

   current_fg = ( ZINT16 ) fg;
   current_bg = ( ZINT16 ) bg;
   if( monochrome)
   {
     attrset ( F_WHITE | B_BLACK );
   }
   else
   {
     attrset (TEXT_ATTR);    
   }

}

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
 *  Line drawing characters (0xb3 - 0xda):
 *
 *  0xb3 vertical line (|)
 *  0xba double vertical line (#)
 *  0xc4 horizontal line (-)
 *  0xcd double horizontal line (=)
 *  all other are corner pieces (+)
 */
int codes_to_text( int c, char *s )
{
   /* German characters need translation */

   if ( c > 154 && c < 224 )
   {
      s[0] = zscii2latin1[c - 155];

      if ( c == 220 )
      {
         s[1] = 'e';
         s[2] = '\0';
      }
      else if ( c == 221 )
      {
         s[1] = 'E';
         s[2] = '\0';
      }
      else
      {
         s[1] = '\0';
      }
      return 0;
   }
   return 1;
}                               /* codes_to_text */
