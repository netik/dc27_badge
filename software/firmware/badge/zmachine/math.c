
/* $Id: math.c,v 1.3 2000/07/05 15:20:34 jholder Exp $   
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information   
 * --------------------------------------------------------------------
 * 
 * File name: $Id: math.c,v 1.3 2000/07/05 15:20:34 jholder Exp $  
 *   
 * Description:    
 *    
 * Modification history:      
 * $Log: math.c,v $
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
 * math.c
 *
 * Arithmetic, compare and logical instructions
 *
 */

#include "ztypes.h"

/*
 * Arduino doesn't have rand() and srand(), lets add them
 */
 
static unsigned long int next = 1;
int random(void) // RAND_MAX assumed to be 32767
{
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % 32768;
}

void srandom(unsigned int seed)
{
    next = seed;
}

/*
 * z_add
 *
 * Add two operands
 *
 */

void z_add( zword_t a, zword_t b )
{
   store_operand( (zword_t)(a + b) );
}

/*
 * z_sub
 *
 * Subtract two operands
 *
 */

void z_sub( zword_t a, zword_t b )
{
   store_operand( (zword_t)(a - b) );
}

/*
 * mul
 *
 * Multiply two operands
 *
 */

void z_mul( zword_t a, zword_t b )
{
   store_operand( (zword_t)(( ZINT16 ) a * ( ZINT16 ) b) );
}

/*
 * z_div
 *
 * Divide two operands
 *
 */

void z_div( zword_t a, zword_t b )
{
   /* The magic rule is: round towards zero. */
   ZINT16 sa = ( ZINT16 ) a;
   ZINT16 sb = ( ZINT16 ) b;
   ZINT16 res;

   if ( sb < 0 )
   {
      sa = -sa;
      sb = -sb;
   }
   if ( sb == 0 )
   {
#ifdef STRICTZ
      report_strictz_error( STRZERR_DIV_ZERO,
                            "@div attempted with a divisor of zero.  Result set to 32767 (0x7fff)." );
#endif
      res = 32767;
   }
   else if ( sa >= 0 )
   {
      res = sa / sb;
   }
   else
   {
      res = -( ( -sa ) / ( sb ) );
   }
   store_operand( res );

}

/*
 * z_mod
 *
 * Modulus divide two operands
 *
 */

void z_mod( zword_t a, zword_t b )
{
   /* The magic rule is: be consistent with divide, because
    * (a/b)*a + (a%b) == a. So (a%b) has the same sign as a. */
   ZINT16 sa = ( ZINT16 ) a;
   ZINT16 sb = ( ZINT16 ) b;
   ZINT16 res;

   if ( sb < 0 )
   {
      sb = -sb;
   }
   if ( sb == 0 )
   {
#ifdef STRICTZ
      report_strictz_error( STRZERR_DIV_ZERO,
                            "@mod attempted with a divisor of zero.  Result set to 0." );
#endif
      res = 0;
   }
   if ( sa >= 0 )
   {
      res = sa % sb;
   }
   else
   {
      res = -( ( -sa ) % ( sb ) );
   }
   store_operand( res );

}

/*
 * z_log_shift
 *
 * Shift +/- n bits
 *
 */

void z_log_shift( zword_t a, zword_t b )
{

   if ( ( ZINT16 ) b > 0 )
      store_operand( (zword_t)( a << ( ZINT16 ) b ) );
   else
      store_operand( (zword_t)( a >> -( ( ZINT16 ) b ) ) );

}


/*
 * z_art_shift
 *
 * Aritmetic shift +/- n bits
 *
 */

void z_art_shift( zword_t a, zword_t b )
{
   if ( ( ZINT16 ) b > 0 )
      store_operand( ( zword_t ) ( ( ZINT16 ) a << ( ZINT16 ) b ) );
   else
      store_operand( ( zword_t ) ( ( ZINT16 ) a >> -( ( ZINT16 ) b ) ) );
}

/*
 * z_or
 *
 * Logical OR
 *
 */

void z_or( zword_t a, zword_t b )
{
   store_operand( (zword_t)(a | b) );
}

/*
 * z_not
 *
 * Logical NOT
 *
 */

void z_not( zword_t a )
{
   store_operand( (zword_t)(~a) );
}

/*
 * z_and
 *
 * Logical AND
 *
 */

void z_and( zword_t a, zword_t b )
{
   store_operand( (zword_t)(a & b) );
}

/*
 * z_random
 *
 * Return random number between 1 and operand
 *
 */

void z_random( zword_t a )
{
   if ( a == 0 )
      store_operand( 0 );
   else if ( a & 0x8000 )
   {                            /* (a < 0) - used to set seed with #RANDOM */
      SRANDOM_FUNC( ( unsigned int ) abs( a ) );
      store_operand( 0 );
   }
   else                         /* (a > 0) */
      store_operand( (zword_t)(( RANDOM_FUNC(  ) % a ) + 1) );
}

/*
 * z_test
 *
 * Jump if operand 2 bit mask not set in operand 1
 *
 */

void z_test( zword_t a, zword_t b )
{
   conditional_jump( ( ( ~a ) & b ) == 0 );
}

/*
 * z_jz
 *
 * Compare operand against zero
 *
 */

void z_jz( zword_t a )
{
   conditional_jump( a == 0 );
}

/*
 * z_je
 *
 * Jump if operand 1 is equal to any other operand
 *
 */

void z_je( int count, zword_t * operand )
{
   int i;

   for ( i = 1; i < count; i++ )
      if ( operand[0] == operand[i] )
      {
         conditional_jump( TRUE );
         return;
      }
   conditional_jump( FALSE );
}

/*
 * z_jl
 *
 * Jump if operand 1 is less than operand 2
 *
 */

void z_jl( zword_t a, zword_t b )
{
   conditional_jump( ( ZINT16 ) a < ( ZINT16 ) b );
}

/*
 * z_jg
 *
 * Jump if operand 1 is greater than operand 2
 *
 */

void z_jg( zword_t a, zword_t b )
{
   conditional_jump( ( ZINT16 ) a > ( ZINT16 ) b );
}
