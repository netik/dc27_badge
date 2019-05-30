
/* $Id: operand.c,v 1.2 2000/05/25 22:28:56 jholder Exp $   
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information   
 * --------------------------------------------------------------------
 * 
 * File name: $Id: operand.c,v 1.2 2000/05/25 22:28:56 jholder Exp $  
 *   
 * Description:    
 *    
 * Modification history:      
 * $Log: operand.c,v $
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
 * operand.c
 *
 * Operand manipulation routines
 *
 */

#include "ztypes.h"

/*
 * load_operand
 *
 * Load an operand, either: a variable, popped from the stack or a literal.
 *
 */

zword_t load_operand( int type )
{
   zword_t operand;

   if ( type )
   {

      /* Type 1: byte literal, or type 2: operand specifier */

      operand = ( zword_t ) read_code_byte(  );
      if ( type == 2 )
      {

         /* If operand specifier non-zero then it's a variable, otherwise
          * it's the top of the stack */

         if ( operand )
         {
            return load_variable( operand );
         }
         else
         {
            return stack[sp++];
         }
      }
   }
   else
   {
      /* Type 0: word literal */
      return read_code_word(  );
   }
   return ( operand );

}                               /* load_operand */

/*
 * store_operand
 *
 * Store an operand, either as a variable pushed on the stack.
 *
 */

void store_operand( zword_t operand )
{
   zbyte_t specifier;

   /* Read operand specifier byte */

   specifier = read_code_byte(  );

   /* If operand specifier non-zero then it's a variable, otherwise it's the
    * pushed on the stack */

   if ( specifier )
      z_store( specifier, operand );
   else
      stack[--sp] = operand;

}                               /* store_operand */

/*
 * load_variable
 *
 * Load a variable, either: a stack local variable, a global variable or
 * the top of the stack.
 *
 */

zword_t load_variable( int number )
{
   if ( number )
   {
      if ( number < 16 )
      {
         /* number in range 1 - 15, it's a stack local variable */
         return stack[fp - ( number - 1 )];
      }
      else
      {
         /* number > 15, it's a global variable */
         return get_word( h_globals_offset + ( ( number - 16 ) * 2 ) );
      }
   }
   else
   {
      /* number = 0, get from top of stack */
      return stack[sp];
   }

}                               /* load_variable */

/*
 * z_store
 *
 * Store a variable, either: a stack local variable, a global variable or the
 * top of the stack.
 *
 */

void z_store( int number, zword_t variable )
{

   if ( number )
   {
      if ( number < 16 )

         /* number in range 1 - 15, it's a stack local variable */

         stack[fp - ( number - 1 )] = variable;
      else
         /* number > 15, it's a global variable */

         set_word( h_globals_offset + ( ( number - 16 ) * 2 ), variable );
   }
   else
      /* number = 0, get from top of stack */

      stack[sp] = variable;

}                               /* z_store */

/*
 * z_piracy
 *
 * Supposed to jump if the game thinks that it is pirated.
 */
void z_piracy( int flag )
{
   conditional_jump( flag );
}

/*
 * conditional_jump
 *
 * Take a jump after an instruction based on the flag, either true or false. The
 * jump can be modified by the change logic flag. Normally jumps are taken
 * when the flag is true. When the change logic flag is set then the jump is
 * taken when flag is false. A PC relative jump can also be taken. This jump can
 * either be a positive or negative byte or word range jump. An additional
 * feature is the return option. If the jump offset is zero or one then that
 * literal value is passed to the return instruction, instead of a jump being
 * taken. Complicated or what!
 *
 */

void conditional_jump( int flag )
{
   zbyte_t specifier;
   zword_t offset;

   /* Read the specifier byte */

   specifier = read_code_byte(  );

   /* If the reverse logic flag is set then reverse the flag */

   if ( specifier & 0x80 )
      flag = ( flag ) ? 0 : 1;

   /* Jump offset is in bottom 6 bits */

   offset = ( zword_t ) specifier & 0x3f;

   /* If the byte range jump flag is not set then load another offset byte */

   if ( ( specifier & 0x40 ) == 0 )
   {

      /* Add extra offset byte to existing shifted offset */

      offset = ( offset << 8 ) + read_code_byte(  );

      /* If top bit of offset is set then propogate the sign bit */

      if ( offset & 0x2000 )
         offset |= 0xc000;
   }

   /* If the flag is false then do the jump */

   if ( flag == 0 )
   {

      /* If offset equals 0 or 1 return that value instead */

      if ( offset == 0 || offset == 1 )
      {
         z_ret( offset );
      }
      else
      {                         /* Add offset to PC */
         pc = ( unsigned long ) ( pc + ( ZINT16 ) offset - 2 );
      }
   }
}                               /* conditional_jump */
