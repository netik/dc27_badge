/* $Id: ztypes.h,v 1.4 2000/10/04 23:07:57 jholder Exp $
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information
 * --------------------------------------------------------------------
 *
 * File name: $Id: ztypes.h,v 1.4 2000/10/04 23:07:57 jholder Exp $
 *
 * Description:
 *
 * Modification history:
 * $Log: ztypes.h,v $
 * Revision 1.4  2000/10/04 23:07:57  jholder
 * fixed redirect problem with isolatin1 range chars
 *
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
 * ztypes.h
 *
 * Any global stuff required by the C modules.
 *
 */

#if !defined(__ZTYPES_INCLUDED)
#define __ZTYPES_INCLUDED

#define _BSD_SOURCE
#define POSIX

/* AIX likes to see this define... */
#if defined(AIX)
#define _POSIX_SOURCE
#define POSIX
#endif

/* for Turbo C & MSC */

#if defined(__MSDOS__)
#ifndef MSDOS
#define MSDOS
#endif
#define LOUSY_RANDOM
#define Z_FILENAME_MAX FILENAME_MAX
#endif

#if defined OS2
#define LOUSY_RANDOM
#define Z_FILENAME_MAX FILENAME_MAX
#endif

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(MSDOS)
#include <malloc.h>
#endif /* MSDOS */

/* Set Version of JZIP */

#include "jzip.h"
extern unsigned char JTERP;

/* Configuration options */

#ifdef USE_ZLIB
#include <zlib.h>
#define jz_rewind  gzrewind
#define jz_seek    gzseek
#define jz_open    gzopen
#define jz_close   gzclose
#define jz_getc    gzgetc
#else
#define jz_rewind  rewind
#define jz_seek    fseek
#define jz_open    fopen
#define jz_close   fclose
#define jz_getc    getc
#endif

#define USE_QUETZAL

#define DEFAULT_ROWS 24         /* Default screen height */
#define DEFAULT_COLS 80         /* Deafult screen width */

#define DEFAULT_RIGHT_MARGIN 1  /* # of characters in rt margin (UNIX likes 1)*/
#define DEFAULT_TOP_MARGIN   0  /* # of lines on screen before [MORE] message */


#ifdef LOUSY_RANDOM
#define RANDOM_FUNC  rand
#define SRANDOM_FUNC srand
#else
#define RANDOM_FUNC  random
#define SRANDOM_FUNC srandom
#endif

/* Perform stricter z-code error checking. If STRICTZ is #defined,
 * the interpreter will check for common opcode errors, such as reading
 * or writing properties of the "nothing" (0) object. When such an
 * error occurs, the opcode will call report_zstrict_error() and
 * then continue in some safe manner. This may mean doing nothing,
 * returning 0, or something like that.
 *
 * See osdepend.c for the definition of report_zstrict_error(). Note that
 * this function may call fatal() to shut down the interpreter.
 *
 * If STRICTZ is not #defined, the STRICTZ patch has no effect at all.
 * It does not even check to continue safely when an error occurs;
 * it just behaves the way ZIP has always behaved. This typically
 * means calling get_property_addr(0) or get_object_address(0),
 * which will return a meaningless value, and continuing on with
 * that.
 */
#define STRICTZ


/* Global defines */

#ifndef UNUSEDVAR
#define UNUSEDVAR(a)  a=a;
#endif

/* number of bits in a byte.  needed by AIX!!! ;^) */
#ifndef NBBY
#define NBBY 8
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef Z_FILENAME_MAX
#define Z_FILENAME_MAX 255
#endif

#ifndef Z_PATHNAME_MAX
#define Z_PATHNAME_MAX 1024
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

#ifdef unix

#if defined (HAVE_BCOPY)
#define memmove(a, b, c) bcopy (b, a, c)
#else
#define memmove(a, b, c) memcpy ((a), (b), (c))
#endif

#ifndef const
#define const
#endif

#endif /* unix */


/* Z types */

typedef unsigned char zbyte_t;  /* unsigned 1 byte quantity */
typedef unsigned short zword_t; /* unsigned 2 byte quantity */
typedef short ZINT16;           /*   signed 2 byte quantity */

/* Data file header format */

typedef struct zheader
{
   zbyte_t type;
   zbyte_t config;
   zword_t version;
   zword_t data_size;
   zword_t start_pc;
   zword_t words_offset;
   zword_t objects_offset;
   zword_t globals_offset;
   zword_t restart_size;
   zword_t flags;
   zbyte_t release_date[6];
   zword_t synonyms_offset;
   zword_t file_size;
   zword_t checksum;
   zbyte_t interpreter;
   zbyte_t interpreter_version;
   zbyte_t screen_rows;
   zbyte_t screen_columns;
   zbyte_t screen_left;
   zbyte_t screen_right;
   zbyte_t screen_top;
   zbyte_t screen_bottom;
   zbyte_t max_char_width;
   zbyte_t max_char_height;
   zword_t filler1[3];
   zword_t function_keys_offset;
   zword_t filler2[2];
   zword_t alternate_alphabet_offset;
   zword_t mouse_position_offset;
   zword_t filler3[4];
}
zheader_t;

#define H_TYPE                 0
#define H_CONFIG               1

#define CONFIG_BYTE_SWAPPED 0x01 /* Game data is byte swapped          - V3   */
#define CONFIG_TIME         0x02 /* Status line displays time          - V3   */
#define CONFIG_MAX_DATA     0x04 /* Data area should 64K if possible   - V3   */
#define CONFIG_TANDY        0x08 /* Tandy licensed game                - V3   */
#define CONFIG_NOSTATUSLINE 0x10 /* Interp can't support a status line - V3   */
#define CONFIG_WINDOWS      0x20 /* Interpr supports split screen mode - V3   */

#define CONFIG_COLOUR       0x01 /* Game supports colour               - V5+  */
#define CONFIG_PICTURES     0x02 /* Picture displaying available?      - V4+  */
#define CONFIG_BOLDFACE     0x04 /* Interpr supports boldface style    - V4+  */
#define CONFIG_EMPHASIS     0x08 /* Interpreter supports text emphasis - V4+  */
#define CONFIG_FIXED        0x10 /* Interpr supports fixed width style - V4+  */
#define CONFIG_SFX          0x20 /* Interpr supports sound effects     - V4+  */
#define CONFIG_TIMEDINPUT   0x80 /* Interpr supports timed input       - V4+  */

#define CONFIG_PROPORTIONAL 0x40 /* Interpr uses proportional font     - V3+  */


#define H_VERSION              2
#define H_DATA_SIZE            4
#define H_START_PC             6
#define H_WORDS_OFFSET         8
#define H_OBJECTS_OFFSET      10
#define H_GLOBALS_OFFSET      12
#define H_RESTART_SIZE        14
#define H_FLAGS               16

#define SCRIPTING_FLAG      0x01
#define FIXED_FONT_FLAG     0x02
#define REFRESH_FLAG        0x04
#define GRAPHICS_FLAG       0x08
#define SOUND_FLAG          0x10 /* V4 */
#define UNDO_AVAILABLE_FLAG 0x10 /* V5 */
#define COLOUR_FLAG         0x40
#define NEW_SOUND_FLAG      0x80

#define H_RELEASE_DATE        18
#define H_SYNONYMS_OFFSET     24
#define H_FILE_SIZE           26
#define H_CHECKSUM            28
#define H_INTERPRETER         30
#define H_UNICODE_TABLE       34

#define INTERP_GENERIC 0
#define INTERP_DEC_20 1
#define INTERP_APPLE_IIE 2
#define INTERP_MACINTOSH 3
#define INTERP_AMIGA 4
#define INTERP_ATARI_ST 5
#define INTERP_MSDOS 6
#define INTERP_CBM_128 7
#define INTERP_CBM_64 8
#define INTERP_APPLE_IIC 9
#define INTERP_APPLE_IIGS 10
#define INTERP_TANDY 11
#define INTERP_UNIX 12
#define INTERP_VMS 13

#define H_INTERPRETER_VERSION 31
#define H_SCREEN_ROWS 32
#define H_SCREEN_COLUMNS 33
#define H_SCREEN_LEFT 34
#define H_SCREEN_RIGHT 35
#define H_SCREEN_TOP 36
#define H_SCREEN_BOTTOM 37
#define H_MAX_CHAR_WIDTH 38
#define H_MAX_CHAR_HEIGHT 39
#define H_FILLER1 40

#define H_FUNCTION_KEYS_OFFSET 46
#define H_FILLER2 48

#define H_STANDARD_HIGH 50
#define H_STANDARD_LOW 51

#define H_ALTERNATE_ALPHABET_OFFSET 52
#define H_MOUSE_POSITION_OFFSET 54
#define H_FILLER3 56

#define V1 1

#define V2 2

/* Version 3 object format */

#define V3 3

typedef struct zobjectv3
{
   zword_t attributes[2];
   zbyte_t parent;
   zbyte_t next;
   zbyte_t child;
   zword_t property_offset;
}
zobjectv3_t;

#define O3_ATTRIBUTES 0
#define O3_PARENT 4
#define O3_NEXT 5
#define O3_CHILD 6
#define O3_PROPERTY_OFFSET 7

#define O3_SIZE 9

#define PARENT3(offset) (offset + O3_PARENT)
#define NEXT3(offset) (offset + O3_NEXT)
#define CHILD3(offset) (offset + O3_CHILD)

#define P3_MAX_PROPERTIES 0x20

/* Version 4 object format */

#define V4 4

typedef struct zobjectv4
{
   zword_t attributes[3];
   zword_t parent;
   zword_t next;
   zword_t child;
   zword_t property_offset;
}
zobjectv4_t;

#define O4_ATTRIBUTES 0
#define O4_PARENT 6
#define O4_NEXT 8
#define O4_CHILD 10
#define O4_PROPERTY_OFFSET 12

#define O4_SIZE 14

#define PARENT4(offset) (offset + O4_PARENT)
#define NEXT4(offset) (offset + O4_NEXT)
#define CHILD4(offset) (offset + O4_CHILD)

#define P4_MAX_PROPERTIES 0x40

#define V5 5
#define V6 6
#define V7 7
#define V8 8

/* Interpreter states */

#define STOP 0
#define RUN 1

/* Call types */

#define FUNCTION 0x0000

#if defined(USE_QUETZAL)
#define PROCEDURE 0x1000
#define ASYNC 0x2000
#else
#define PROCEDURE 0x0100
#define ASYNC 0x0200
#endif

#if defined(USE_QUETZAL)
#define ARGS_MASK 0x00FF
#define VARS_MASK 0x0F00
#define TYPE_MASK 0xF000
#define VAR_SHIFT 8
#else
#define ARGS_MASK 0x00ff
#define TYPE_MASK 0xff00
#endif

/* Local defines */

#define PAGE_SIZE 0x200
#define PAGE_MASK 0x1FF
#define PAGE_SHIFT 9

#define STACK_SIZE 1024

#define ON     1
#define OFF    0
#define RESET -1

#define Z_SCREEN      255
#define TEXT_WINDOW   0
#define STATUS_WINDOW 1

#define MIN_ATTRIBUTE 0
#define NORMAL        0
#define REVERSE       1
#define BOLD          2
#define EMPHASIS      4
#define FIXED_FONT    8
#define MAX_ATTRIBUTE 8

#define TEXT_FONT     1
#define GRAPHICS_FONT 3

#define FOREGROUND    0
#define BACKGROUND    1

#define GAME_RESTORE  0
#define GAME_SAVE     1
#define GAME_SCRIPT   2
#define GAME_RECORD   3
#define GAME_PLAYBACK 4
#define UNDO_SAVE     5
#define UNDO_RESTORE  6
#define GAME_SAVE_AUX 7
#define GAME_LOAD_AUX 8

#define MAX_TEXT_SIZE 8

/* Data access macros */

#define get_byte(offset) ((zbyte_t) datap[offset])
#define get_word(offset) ((zword_t) (((zword_t) datap[offset] << 8) + (zword_t) datap[offset + 1]))
#define set_byte(offset,value) datap[offset] = (zbyte_t) (value)
#define set_word(offset,value) datap[offset] = (zbyte_t) ((zword_t) (value) >> 8), datap[offset + 1] = (zbyte_t) ((zword_t) (value) & 0xff)

/* External data */

extern int GLOBALVER;
extern zbyte_t h_type;
extern zbyte_t h_config;
extern zword_t h_version;
extern zword_t h_data_size;
extern zword_t h_start_pc;
extern zword_t h_words_offset;
extern zword_t h_objects_offset;
extern zword_t h_globals_offset;
extern zword_t h_restart_size;
extern zword_t h_flags;
extern zword_t h_synonyms_offset;
extern zword_t h_file_size;
extern zword_t h_checksum;
extern zbyte_t h_interpreter;
extern zbyte_t h_interpreter_version;
extern zword_t h_alternate_alphabet_offset;
extern zword_t h_unicode_table;

extern int story_scaler;
extern int story_shift;
extern int property_mask;
extern int property_size_mask;

extern zword_t stack[STACK_SIZE];
extern zword_t sp;
extern zword_t fp;
extern zword_t frame_count;
extern unsigned long pc;
extern int interpreter_state;
extern int interpreter_status;

extern unsigned int data_size;
extern zbyte_t *datap;
extern zbyte_t *undo_datap;

extern int screen_rows;
extern int screen_cols;
extern int right_margin;
extern int top_margin;

extern int screen_window;
extern int interp_initialized;

extern int formatting;
extern int outputting;
extern int redirect_depth;
extern int redirecting;
extern int scripting;
extern int scripting_disable;
extern int recording;
extern int replaying;
extern int font;

extern int status_active;
extern int status_size;

extern char fTandy;
extern char fIBMGraphics;

extern int lines_written;
extern int status_pos;

extern char *line;
extern char *status_line;

extern char lookup_table[3][26];

extern char monochrome;
extern int hist_buf_size;
extern char bigscreen;

extern unsigned char zscii2latin1[69];

#ifdef STRICTZ

/* Definitions for STRICTZ functions and error codes. */

void report_strictz_error( int, const char * );

/* Error codes */
#define STRZERR_NO_ERROR       (0)
#define STRZERR_JIN            (1)
#define STRZERR_GET_CHILD      (2)
#define STRZERR_GET_PARENT     (3)
#define STRZERR_GET_SIBLING    (4)
#define STRZERR_GET_PROP_ADDR  (5)
#define STRZERR_GET_PROP       (6)
#define STRZERR_PUT_PROP       (7)
#define STRZERR_CLEAR_ATTR     (8)
#define STRZERR_SET_ATTR       (9)
#define STRZERR_TEST_ATTR     (10)
#define STRZERR_MOVE_OBJECT   (11)
#define STRZERR_MOVE_OBJECT_2 (12)
#define STRZERR_REMOVE_OBJECT (13)
#define STRZERR_GET_NEXT_PROP (14)
#define STRZERR_DIV_ZERO      (15)
#define STRZERR_MOV_CURSOR    (16)
#define STRICTZ_NUM_ERRORS    (17)

#endif /* STRICTZ */

/* Global routines */

/* control.c */

void z_check_arg_count( zword_t );
int  z_call( int, zword_t *, int );
void z_jump( zword_t );
void z_restart( void );
void restart_interp( int );
void z_ret( zword_t );
void z_catch( void );
void z_throw( zword_t, zword_t );


/* fileio.c */

void z_verify( void );
int z_restore( int, zword_t, zword_t, zword_t );
int z_save( int, zword_t, zword_t, zword_t );
void z_restore_undo( void );
void z_save_undo( void );
void z_open_playback( int );
void close_record( void );
void close_script( void );
void close_story( void );
void flush_script( void );
unsigned int get_story_size( void );
void open_record( void );
void open_script( void );
void open_story( const char * );
int playback_key( void );
int playback_line( int, char *, int * );
void read_page( int, void * );
void record_key( int );
void record_line( const char * );
void script_char( int );
void script_string( const char * );
void script_line( const char * );
void script_new_line( void );


/* getopt.c */

#ifndef HAVE_GETOPT
int getopt( int, char *[], const char * );
#endif


/* input.c */

int get_line( char *, zword_t, zword_t );
void z_read_char( int, zword_t * );
void z_sread_aread( int, zword_t * );
void z_tokenise( int, zword_t * );


/* interpre.c */

int interpret( void );


/* license.c */

void print_license( void );


/* math.c */

void z_add( zword_t, zword_t );
void z_div( zword_t, zword_t );
void z_mul( zword_t, zword_t );
void z_sub( zword_t, zword_t );
void z_mod( zword_t, zword_t );
void z_or( zword_t, zword_t );
void z_and( zword_t, zword_t );
void z_not( zword_t );
void z_art_shift( zword_t, zword_t );
void z_log_shift( zword_t, zword_t );
void z_je( int, zword_t * );
void z_jg( zword_t, zword_t );
void z_jl( zword_t, zword_t );
void z_jz( zword_t );
void z_random( zword_t );
void z_test( zword_t, zword_t );


/* memory.c */

void load_cache( void );
void unload_cache( void );
zbyte_t read_code_byte( void );
zbyte_t read_data_byte( unsigned long * );
zword_t read_code_word( void );
zword_t read_data_word( unsigned long * );


/* object.c */

zword_t get_object_address( zword_t );
void z_insert_obj( zword_t, zword_t );
void z_remove_obj( zword_t );
void z_get_child( zword_t );
void z_get_sibling( zword_t );
void z_get_parent( zword_t );
void z_jin( zword_t, zword_t );
void z_clear_attr( zword_t, zword_t );
void z_set_attr( zword_t, zword_t );
void z_test_attr( zword_t, zword_t );


/* operand.c */

void z_piracy( int );
void z_store( int, zword_t );
void conditional_jump( int );
void store_operand( zword_t );
zword_t load_operand( int );
zword_t load_variable( int );


/* osdepend.c */

int codes_to_text( int, char * );
void fatal( const char * );
void file_cleanup( const char *, int );
int fit_line( const char *, int, int );
int get_file_name( char *, char *, int );
int print_status( int, char *[] );
void process_arguments( int, char *[] );
void set_colours( zword_t, zword_t );
void set_font( int );
void sound( int, zword_t * );


/* property.c */

void z_get_next_prop( zword_t, zword_t );
void z_get_prop( zword_t, zword_t );
void z_get_prop_addr( zword_t, zword_t );
void z_get_prop_len( zword_t );
void z_put_prop( zword_t, zword_t, zword_t );
void z_copy_table( zword_t, zword_t, zword_t );
void z_scan_table( int, zword_t * );
void z_loadb( zword_t, zword_t );
void z_loadw( zword_t, zword_t );
void z_storeb( zword_t, zword_t, zword_t );
void z_storew( zword_t, zword_t, zword_t );


/* quetzal.c */

#ifdef USE_ZLIB
int save_quetzal( FILE *, gzFile * );
int restore_quetzal( FILE *, gzFile * );
#else
int save_quetzal( FILE *, FILE *);
int restore_quetzal( FILE *, FILE *);
#endif


/* screen.c */

void z_show_status( void );
void z_set_cursor( zword_t, zword_t );
void z_set_font( zword_t );
void z_split_window( zword_t );
void z_set_window( zword_t );
void z_set_colour( zword_t, zword_t );
void z_erase_line( zword_t );
void z_erase_window( zword_t );
void z_print_table( int, zword_t * );
void blank_status_line( void );
void output_char( int );
void output_new_line( void );
void output_string( const char * );
void output_line( const char * );


/* screenio.c */

int input_character( int );
void clear_line( void );
void clear_screen( void );
void clear_status_window( void );
void clear_text_window( void );
void create_status_window( void );
void delete_status_window( void );
void display_char( int );
int fit_line( const char *, int, int );
void get_cursor_position( int *, int * );
void initialize_screen( void );
int input_line( int, char *, int, int * );
void move_cursor( int, int );
int print_status( int, char *[] );
void reset_screen( void );
void restart_screen( void );
void restore_cursor_position( void );
void save_cursor_position( void );
void scroll_line( void );
void select_status_window( void );
void select_text_window( void );
void set_attribute( int );


/* text.c */

void z_encode( zword_t, zword_t, zword_t, zword_t );
void z_new_line( void );
void z_print_char( zword_t );
void z_print_num( zword_t );
void z_print( void );
void z_print_addr( zword_t );
void z_print_paddr( zword_t );
void z_print_obj( zword_t );
void z_print_ret( void );
void z_buffer_mode( zword_t );
void z_output_stream( zword_t, zword_t );
void z_input_stream( int );
void z_set_text_style( zword_t );
void decode_text( unsigned long * );
void encode_text( int, const char *, ZINT16 * );
void flush_buffer( int );
void print_time( int, int );
void write_char( int );
void write_string( const char * );
void write_zchar( int );


/* variable.c */

void z_inc( zword_t );
void z_dec( zword_t );
void z_inc_chk( zword_t, zword_t );
void z_dec_chk( zword_t, zword_t );
void z_load( zword_t );
void z_pull( zword_t );
void z_push( zword_t );

/* Arduino stuff */

#define USE_MCURSES_H
#define GAMEPATH "/stories"
#define SAVEPATH "/saves"
#define FILE_CREATE (FA_READ | FA_WRITE | FA_CREATE_ALWAYS)
#define STATUS_ATTR (F_WHITE | B_BLUE | A_BOLD)
#define TEXT_ATTR (F_GREEN | B_BLACK)

typedef struct ztheme
{
  const char *tname;
  uint16_t status_attr;
  uint16_t text_attr;
}
ztheme_t;

#endif /* !defined(__ZTYPES_INCLUDED) */
