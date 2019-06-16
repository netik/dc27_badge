
/* $Id: control.c,v 1.2 2000/05/25 22:28:56 jholder Exp $   
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information   
 * --------------------------------------------------------------------
 * 
 * File name: $Id: control.c,v 1.2 2000/05/25 22:28:56 jholder Exp $  
 *   
 * Description:    
 *    
 * Modification history:      
 * $Log: control.c,v $
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
 * control.c
 *
 * Functions that alter the flow of control.
 *
 */

#include "ztypes.h"

static const char *v1_lookup_table[3] = {
   "abcdefghijklmnopqrstuvwxyz",
   "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
   " 0123456789.,!?_#'\"/\\<-:()"
};

static const char *v3_lookup_table[3] = {
   "abcdefghijklmnopqrstuvwxyz",
   "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
   " \n0123456789.,!?_#'\"/\\-:()"
};

/*
 * z_check_arg_count
 *
 * Jump if argument is present.
 *
 */

void z_check_arg_count( zword_t argc )
{

   conditional_jump( argc <= ( zword_t ) ( stack[fp + 1] & ARGS_MASK ) );

}                               /* z_check_arg_count */

/*
 * z_call
 *
 * Call a subroutine. Save PC and FP then load new PC and initialise stack based
 * local arguments.
 *
 * Implements: call_1s, call_1n, call_2s, call_2n, call, call_vs, call_vs2, call_vn, call_vn2
 *
 */

int z_call( int argc, zword_t * argv, int type )
{
   zword_t arg;
   int i = 1, args, status = 0;

   /* Convert calls to 0 as returning FALSE */

   if ( argv[0] == 0 )
   {
      if ( type == FUNCTION )
         store_operand( FALSE );
      return ( 0 );
   }

   /* Save current PC, FP and argument count on stack */

   stack[--sp] = ( zword_t ) ( pc / PAGE_SIZE );
   stack[--sp] = ( zword_t ) ( pc % PAGE_SIZE );
   stack[--sp] = fp;
   stack[--sp] = ( argc - 1 ) | type;

   /* Create FP for new subroutine and load new PC */

   fp = sp - 1;
   pc = ( unsigned long ) argv[0] * story_scaler;
#if defined(USE_QUETZAL)
   ++frame_count;
#endif

   /* Read argument count and initialise local variables */

   args = ( unsigned int ) read_code_byte(  );
#if defined(USE_QUETZAL)
   stack[sp] |= args << VAR_SHIFT;
#endif
   while ( --args >= 0 )
   {
      arg = ( h_type > V4 ) ? 0 : read_code_word(  );
      stack[--sp] = ( --argc > 0 ) ? argv[i++] : arg;
   }

   /* If the call is asynchronous then call the interpreter directly.
    * We will return back here when the corresponding return frame is
    * encountered in the ret call. */

   if ( type == ASYNC )
   {
      status = interpret(  );
      interpreter_state = RUN;
      interpreter_status = 1;
   }

   return ( status );

}                               /* z_call */

/*
 * z_ret
 *
 * Return from subroutine. Restore FP and PC from stack.
 *
 */

void z_ret( zword_t value )
{
   zword_t argc;

   /* Clean stack */

   sp = fp + 1;

   /* Restore argument count, FP and PC */

   argc = stack[sp++];
   fp = stack[sp++];
   pc = stack[sp++];
   pc += ( unsigned long ) stack[sp++] * PAGE_SIZE;
#if defined(USE_QUETZAL)
   --frame_count;
#endif

   /* If this was an async call then stop the interpreter and return
    * the value from the async routine. This is slightly hacky using
    * a global state variable, but ret can be called with conditional_jump
    * which in turn can be called from all over the place, sigh. A
    * better design would have all opcodes returning the status RUN, but
    * this is too much work and makes the interpreter loop look ugly */

   if ( ( argc & TYPE_MASK ) == ASYNC )
   {
      interpreter_state = STOP;
      interpreter_status = ( int ) value;
   }
   else
   {
      /* Return subroutine value for function call only */
      if ( ( argc & TYPE_MASK ) == FUNCTION )
      {
         store_operand( value );
      }
   }

}                               /* z_ret */

/*
 * z_jump
 *
 * Unconditional jump. Jump is PC relative.
 *
 */

void z_jump( zword_t offset )
{

   pc = ( unsigned long ) ( pc + ( ZINT16 ) offset - 2 );

}                               /* z_jump */

/*
 * z_restart
 *
 * Restart game by initialising environment and reloading start PC.
 *
 */

void z_restart( void )
{
   unsigned int i, j, restart_size, scripting_flag;

   /* Reset output buffer */

   flush_buffer( TRUE );

   /* Reset text control flags */

   formatting = ON;
   outputting = ON;
   redirecting = OFF;
   redirect_depth = 0;
   scripting_disable = OFF;

   /* Randomise */

   SRANDOM_FUNC( ( unsigned int ) time( NULL ) );

   /* Remember scripting state */

   scripting_flag = get_word( H_FLAGS ) & SCRIPTING_FLAG;

   /* Load restart size and reload writeable data area */

   restart_size = ( h_restart_size / PAGE_SIZE ) + 1;
   for ( i = 0; i < restart_size; i++ )
   {
      read_page( i, &datap[i * PAGE_SIZE] );
   }

   /* Restart the screen */

   z_split_window( 0 );
   set_colours( 1, 1 );         /* set default colors, added by JDH 8/6/95 */
   set_attribute( NORMAL );
   z_erase_window( Z_SCREEN );

   restart_screen(  );

   /* Reset the interpreter state */

   restart_interp( scripting_flag );

   /* Initialise the character translation lookup tables */

   for ( i = 0; i < 3; i++ )
   {
      for ( j = 0; j < 26; j++ )
      {
         if ( h_alternate_alphabet_offset )
         {
            lookup_table[i][j] = get_byte( h_alternate_alphabet_offset + ( i * 26 ) + j );
         }
         else
         {
            if ( h_type == V1 )
            {
               lookup_table[i][j] = v1_lookup_table[i][j];
            }
            else
            {
               lookup_table[i][j] = v3_lookup_table[i][j];
            }
         }
      }
   }

   /* Load start PC, SP and FP */

   pc = h_start_pc;
   sp = STACK_SIZE;
   fp = STACK_SIZE - 1;
#if defined (USE_QUETZAL)
   frame_count = 0;
#endif

}                               /* z_restart */


/*
 * restart_interp
 *
 * Do all the things which need to be done after startup, restart, and restore
 * commands.
 *
 */

void restart_interp( int scripting_flag )
{
   if ( scripting_flag )
      set_word( H_FLAGS, ( get_word( H_FLAGS ) | SCRIPTING_FLAG ) );

   set_byte( H_INTERPRETER, h_interpreter );
   set_byte( H_INTERPRETER_VERSION, h_interpreter_version );
   set_byte( H_SCREEN_ROWS, screen_rows ); /* Screen dimension in characters */
   set_byte( H_SCREEN_COLUMNS, screen_cols );

   set_byte( H_SCREEN_LEFT, 0 ); /* Screen dimension in smallest addressable units, ie. pixels */
   set_byte( H_SCREEN_RIGHT, screen_cols );
   set_byte( H_SCREEN_TOP, 0 );
   set_byte( H_SCREEN_BOTTOM, screen_rows );

   set_byte( H_MAX_CHAR_WIDTH, 1 ); /* Size of a character in screen units */
   set_byte( H_MAX_CHAR_HEIGHT, 1 );

   /* Initialise status region */

   if ( h_type < V4 )
   {
      z_split_window( 0 );
      blank_status_line(  );
   }

   if ( h_type == V3 && fTandy )
   {                            
      zbyte_t config_byte = get_byte( H_CONFIG ); 

      config_byte |= CONFIG_TANDY; 
      set_byte( H_CONFIG, config_byte ); 
   }                            

}                               /* restart_interp */

/*
 * z_catch
 *
 * Return the value of the frame pointer (FP) for later use with throw.
 * Before V5 games this was a simple pop.
 *
 */

void z_catch( void )
{
   if ( h_type > V4 )
   {
#if defined (USE_QUETZAL)
      store_operand( frame_count );
#else
      store_operand( fp );
#endif
   }
   else
   {
      sp++;
   }
}                               /* z_catch */

/*
 * z_throw
 *
 * Remove one or more stack frames and return. Works like longjmp, see z_catch.
 *
 */

void z_throw( zword_t value, zword_t new_fp )
{

   if ( new_fp > fp )
   {
      fatal( "z_throw(): nonexistant frame" );
   }
#if defined (USE_QUETZAL)
   for ( ; new_fp < frame_count; --frame_count )
   {
      sp = fp + 1;
      fp = stack[sp + 1];
   }
#else
   fp = new_fp;
#endif

   z_ret( value );

}                               /* z_throw */
