
/* $Id: variable.c,v 1.3 2000/07/05 15:20:34 jholder Exp $   
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information   
 * --------------------------------------------------------------------
 * 
 * File name: $Id: variable.c,v 1.3 2000/07/05 15:20:34 jholder Exp $  
 *
 * Description:    
 *    
 * Modification history:      
 * $Log: variable.c,v $
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
 * variable.c
 *
 * Variable manipulation routines
 *
 */

#include "ztypes.h"

/*
 * z_load
 *
 * Load and store a variable value on stack.
 *
 */

void z_load( zword_t variable )
{
   store_operand( load_variable( variable ) );
}                               /* load */

/*
 * z_push
 *
 * Push a value onto the stack
 *
 */

void z_push( zword_t value )
{
   stack[--sp] = value;
}                               /* push_var */

/*
 * z_pull
 *
 * Pop a variable from the stack.
 *
 */

void z_pull( zword_t variable )
{
   z_store( variable, stack[sp++] );
}                               /* pop_var */

/*
 * z_inc
 *
 * Increment a variable.
 *
 */

void z_inc( zword_t variable )
{
   z_store( variable, (zword_t)(load_variable( variable ) + 1) );
}                               /* increment */

/*
 * z_dec
 *
 * Decrement a variable.
 *
 */

void z_dec( zword_t variable )
{
   z_store( variable, (zword_t)(load_variable( variable ) - 1) );
}                               /* decrement */

/*
 * z_inc_chk
 *
 * Increment a variable and then check its value against a target.
 *
 */

void z_inc_chk( zword_t variable, zword_t target )
{
   ZINT16 value;

   value = ( ZINT16 ) load_variable( variable );
   z_store( variable, ++value );
   conditional_jump( value > ( ZINT16 ) target );
}                               /* increment_check */

/*
 * z_dec_chk
 *
 * Decrement a variable and then check its value against a target.
 *
 */

void z_dec_chk( zword_t variable, zword_t target )
{
   ZINT16 value;

   value = ( ZINT16 ) load_variable( variable );
   z_store( variable, --value );
   conditional_jump( value < ( ZINT16 ) target );
}                               /* decrement_check */
