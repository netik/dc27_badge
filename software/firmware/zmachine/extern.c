
/* $Id: extern.c,v 1.2 2000/10/04 23:07:57 jholder Exp $   
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information   
 * --------------------------------------------------------------------
 * 
 * File name: $Id: extern.c,v 1.2 2000/10/04 23:07:57 jholder Exp $  
 *   
 * Description:    
 *    
 * Modification history:      
 * $Log: extern.c,v $
 * Revision 1.2  2000/10/04 23:07:57  jholder
 * fixed redirect problem with isolatin1 range chars
 *
 * Revision 1.1.1.1  2000/05/10 14:21:34  jholder
 *
 * imported
 *
 *
 * --------------------------------------------------------------------
 */

/*
 * extern.c
 *
 * Global data.
 *
 */

#include "ztypes.h"

unsigned char JTERP;
int GLOBALVER;

/* Stuff for Latin-1 Charset */
unsigned char zscii2latin1[69] =  {
   0xe4, 0xf6, 0xfc, 0xc4, 0xd6, 0xdc, 0xdf, 0xbb,
   0xab, 0xeb, 0xef, 0xff, 0xcb, 0xcf, 0xe1, 0xe9,
   0xed, 0xf3, 0xfa, 0xfd, 0xc1, 0xc9, 0xcd, 0xd3,
   0xda, 0xdd, 0xe0, 0xe8, 0xec, 0xf2, 0xf9, 0xc0,
   0xc8, 0xcc, 0xd2, 0xd9, 0xe2, 0xea, 0xee, 0xf4,
   0xfb, 0xc2, 0xca, 0xce, 0xd4, 0xdb, 0xe5, 0xc5,
   0xf8, 0xd8, 0xe3, 0xf1, 0xf5, 0xc3, 0xd1, 0xd5,
   0xe6, 0xc6, 0xe7, 0xc7, 0xfe, 0xf0, 0xde, 0xd0,
   0xa3, 'o', 'O', 0xa1, 0xbf
};

/* Game header data */

zbyte_t h_type = 0;
zbyte_t h_config = 0;
zword_t h_version = 0;
zword_t h_data_size = 0;
zword_t h_start_pc = 0;
zword_t h_words_offset = 0;
zword_t h_objects_offset = 0;
zword_t h_globals_offset = 0;
zword_t h_restart_size = 0;
zword_t h_flags = 0;
zword_t h_synonyms_offset = 0;
zword_t h_file_size = 0;
zword_t h_checksum = 0;
zbyte_t h_interpreter = INTERP_MSDOS;
zbyte_t h_interpreter_version = 'B'; /* Interpreter version 2 */
zword_t h_alternate_alphabet_offset = 0;
zword_t h_unicode_table = 0;

/* Game version specific data */

int story_scaler = 0;
int story_shift = 0;
int property_mask = 0;
int property_size_mask = 0;

/* Stack and PC data */

zword_t stack[STACK_SIZE];
zword_t sp = STACK_SIZE;
zword_t fp = STACK_SIZE - 1;
zword_t frame_count = 0;        /* frame pointer for get_fp */
unsigned long pc = 0;
int interpreter_state = RUN;
int interpreter_status = 0;

/* Data region data */

unsigned int data_size = 0;
zbyte_t *datap = NULL;
zbyte_t *undo_datap = NULL;

/* Screen size data */

int screen_rows = 0;
int screen_cols = 0;
int right_margin = DEFAULT_RIGHT_MARGIN;
int top_margin = DEFAULT_TOP_MARGIN;
char bigscreen = 0;
char monochrome = 0;
int hist_buf_size;

/* Current window data */

int screen_window = TEXT_WINDOW;
int interp_initialized = 0;

/* Formatting and output control data */

int gFontNum = 0;
int formatting = ON;
int outputting = ON;
int redirecting = OFF;
int redirect_depth = 0;         /* 1 or higher means ON */
int scripting_disable = OFF;
int scripting = OFF;
int recording = OFF;
int replaying = OFF;
int font = 1;
int use_bg_color = 1;
ZINT16 default_fg = 9, default_bg = 6;

/* Status region data */

int status_active = OFF;
int status_size = 0;

/* Tandy bit requested */

char fTandy = 0;                

/* IBM (not international) glyphs requested */

char fIBMGraphics = 0;          

/* Text output buffer data */

int lines_written = 0;
int status_pos = 0;

/* Dynamic buffer data */

char *line = NULL;
char *status_line = NULL;

/* Character translation tables */

char lookup_table[3][26];
