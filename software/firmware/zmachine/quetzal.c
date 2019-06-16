
/* $Id: quetzal.c,v 1.3 2000/07/05 15:20:34 jholder Exp $   
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information   
 * --------------------------------------------------------------------
 * 
 * File name: $Id: quetzal.c,v 1.3 2000/07/05 15:20:34 jholder Exp $  
 *   
 * Description:    
 *    
 * Modification history:      
 * $Log: quetzal.c,v $
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

/* quetzal.c
 *
 * routines to handle QUETZAL save format
 */

#include <stdio.h>
#include "ztypes.h"

/* You may want to define these as getc and putc, but then the code gets
 * quite big (especially for put_c).
 */
#define get_c getc
#define put_c fputc

/* Some systems appear to have this in funny places, rather than in <stdio.h>
 * where it should be.
 */
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

#if defined(USE_QUETZAL)        /* don't compile anything otherwise */

typedef unsigned long ul_t;

/* IDs of chunks we understand */
#define ID_FORM 0x464f524d
#define ID_IFZS 0x49465a53
#define ID_IFhd 0x49466864
#define ID_UMem 0x554d656d
#define ID_CMem 0x434d656d
#define ID_Stks 0x53746b73
#define ID_ANNO 0x414e4e4f

/* macros to write QUETZAL files */
#define write_byte(fp,b) (put_c ((unsigned)(b),fp) != EOF) 
#define write_bytx(fp,b) write_byte(fp,(b) & 0xFF)
#define write_word(fp,w) \
    (write_bytx(fp,(w)>> 8) && write_bytx(fp,(w)))
#define write_long(fp,l) \
    (write_bytx(fp,(ul_t)(l)>>24) && write_bytx(fp,(ul_t)(l)>>16) && \
     write_bytx(fp,(ul_t)(l)>> 8) && write_bytx(fp,(ul_t)(l))) 
#define write_chnk(fp,id,len) \
    (write_long(fp,id)      && write_long(fp,len))
#define write_run(fp,run) \
    (write_byte(fp,0)       && write_byte(fp,(run)))

/* save_quetzal
 *
 * attempt to save game in QUETZAL format; return TRUE on success
 */

#ifdef USE_ZLIB
int save_quetzal( FILE * sfp, gzFile * gfp )
#else
int save_quetzal( FILE * sfp, FILE * gfp )
#endif
{
   ul_t ifzslen = 0, cmemlen = 0, stkslen = 0, tmp_pc;
   int c;
   zword_t i, j, n, init_fp, tmp_fp, nstk, nvars, args;
   zword_t frames[STACK_SIZE / 4 + 1];
   zbyte_t var;
   long cmempos, stkspos;

   /* write IFZS header */
   if ( !write_chnk( sfp, ID_FORM, 0 ) )
      return FALSE;
   if ( !write_long( sfp, ID_IFZS ) )
      return FALSE;

   /* write IFhd chunk */
   if ( !write_chnk( sfp, ID_IFhd, 13 ) )
      return FALSE;
   if ( !write_word( sfp, h_version ) )
      return FALSE;
   for ( i = 0; i < 6; ++i )
      if ( !write_byte( sfp, get_byte( H_RELEASE_DATE + i ) ) )
         return FALSE;
   if ( !write_word( sfp, h_checksum ) )
      return FALSE;
   if ( !write_long( sfp, ( ( ul_t ) pc ) << 8 ) ) /* includes pad byte */
      return FALSE;             

   /* write CMem chunk */
   /* j is current run length */
   if ( ( cmempos = ftell( sfp ) ) < 0 )
      return FALSE;
   if ( !write_chnk( sfp, ID_CMem, 0 ) )
      return FALSE;
   jz_rewind( gfp );
   for ( i = 0, j = 0, cmemlen = 0; i < h_restart_size; ++i )
   {
      if ( ( c = jz_getc( gfp ) ) == EOF )
         return FALSE;
      c ^= get_byte( i );
      if ( c == 0 )
         ++j;
      else
      {
         /* write any run there may be */
         if ( j > 0 )
         {
            for ( ; j > 0x100; j -= 0x100 )
            {
               if ( !write_run( sfp, 0xFF ) )
                  return FALSE;
               cmemlen += 2;
            }
            if ( !write_run( sfp, j - 1 ) )
               return FALSE;
            cmemlen += 2;
            j = 0;
         }
         /* write this byte */
         if ( !write_byte( sfp, c ) )
            return FALSE;
         ++cmemlen;
      }
   }
   /* there may be a run here, which we ignore */
   if ( cmemlen & 1 )           /* chunk length must be even */
      if ( !write_byte( sfp, 0 ) )
         return FALSE;

   /* write Stks chunk */
   if ( ( stkspos = ftell( sfp ) ) < 0 )
      return FALSE;
   if ( !write_chnk( sfp, ID_Stks, 0 ) )
      return FALSE;

   /* frames is a list of FPs, most recent first */
   frames[0] = sp - 5;          /* what FP would be if we did a call now */
   for ( init_fp = fp, n = 0; init_fp <= STACK_SIZE - 5; init_fp = stack[init_fp + 2] )
      frames[++n] = init_fp;
   init_fp = frames[n] + 4;

   if ( h_type != 6 )
   {                            /* write a dummy frame for stack used before first call */
      for ( i = 0; i < 6; ++i )
         if ( !write_byte( sfp, 0 ) )
            return FALSE;
      nstk = STACK_SIZE - 1 - init_fp;
      if ( !write_word( sfp, nstk ) )
         return FALSE;
      for ( i = STACK_SIZE - 1; i > init_fp; --i )
         if ( !write_word( sfp, stack[i] ) )
            return FALSE;
      stkslen = 8 + 2 * nstk;
   }
   for ( i = n; i > 0; --i )
   {
      /* write out one stack frame.
       *
       *  tmp_fp : FP when this frame was current
       *  tmp_pc : PC on return from this frame, plus 000pvvvv
       *  nvars  : number of local vars for this frame
       *  args   : argument mask for this frame
       *  nstk   : words of evaluation stack used for this frame
       *  var    : variable to store result
       */
      tmp_fp = frames[i];
      nvars = ( stack[tmp_fp + 1] & VARS_MASK ) >> VAR_SHIFT;
      args = stack[tmp_fp + 1] & ARGS_MASK;
      nstk = tmp_fp - frames[i - 1] - nvars - 4;
      tmp_pc = stack[tmp_fp + 3] + ( ul_t ) stack[tmp_fp + 4] * PAGE_SIZE;
      switch ( stack[tmp_fp + 1] & TYPE_MASK )
      {
         case FUNCTION:
            var = read_data_byte( &tmp_pc ); /* also increments tmp_pc */
            tmp_pc = ( tmp_pc << 8 ) | nvars;
            break;
         case PROCEDURE:
            var = 0;
            tmp_pc = ( tmp_pc << 8 ) | 0x10 | nvars; /* set procedure flag */
            break;
            /* case ASYNC: */
         default:
            output_line( "Illegal Z-machine operation: can't save while in interrupt." );
            return FALSE;
      }
      if ( args != 0 )
         args = ( 1 << args ) - 1; /* make args into bitmap */
      if ( !write_long( sfp, tmp_pc ) )
         return FALSE;
      if ( !write_byte( sfp, var ) )
         return FALSE;
      if ( !write_byte( sfp, args ) )
         return FALSE;
      if ( !write_word( sfp, nstk ) )
         return FALSE;
      for ( j = 0; j < nvars + nstk; ++j, --tmp_fp )
         if ( !write_word( sfp, stack[tmp_fp] ) )
            return FALSE;
      stkslen += 8 + 2 * ( nvars + nstk );
   }

   /* fill in lengths for variable-sized chunks */
   ifzslen = 3 * 8 + 4 + 14 + cmemlen + stkslen;
   if ( cmemlen & 1 )
      ++ifzslen;
   ( void ) fseek( sfp, ( long ) 4, SEEK_SET );
   if ( !write_long( sfp, ifzslen ) )
      return FALSE;
   ( void ) fseek( sfp, cmempos + 4, SEEK_SET );
   if ( !write_long( sfp, cmemlen ) )
      return FALSE;
   ( void ) fseek( sfp, stkspos + 4, SEEK_SET );
   if ( !write_long( sfp, stkslen ) )
      return FALSE;

   return TRUE;
}

/* end save_quetzal */

/* read_word
 *
 * attempt to read a word; return TRUE on success
 */

static int read_word( FILE * fp, zword_t * result )
{
   int a, b;

   if ( ( a = get_c( fp ) ) == EOF )
      return FALSE;
   if ( ( b = get_c( fp ) ) == EOF )
      return FALSE;
   *result = ( ( zword_t ) a << 8 ) | b;
   return TRUE;
}

/* read_long
 *
 * attempt to read a longword; return TRUE on success
 */

static int read_long( FILE * fp, ul_t * result )
{
   int a, b, c, d;

   if ( ( a = get_c( fp ) ) == EOF )
      return FALSE;
   if ( ( b = get_c( fp ) ) == EOF )
      return FALSE;
   if ( ( c = get_c( fp ) ) == EOF )
      return FALSE;
   if ( ( d = get_c( fp ) ) == EOF )
      return FALSE;
   *result = ( ( ul_t ) a << 24 ) | ( ( ul_t ) b << 16 ) | ( ( ul_t ) c << 8 ) | d;
#ifdef QDEBUG
   printf( "%c%c%c%c", a, b, c, d );
#endif
   return TRUE;
}

/* restore_quetzal
 *
 * attempt to restore game in QUETZAL format; return TRUE on success
 */

#define GOT_HEADER	0x01
#define GOT_STACK	0x02
#define GOT_MEMORY	0x04
#define GOT_NONE	0x00
#define GOT_ALL		0x07
#define GOT_ERROR	0x80

#ifdef USE_ZLIB
int restore_quetzal( FILE * sfp, gzFile * gfp )
#else
int restore_quetzal( FILE * sfp, FILE * gfp )
#endif
{
   ul_t ifzslen, currlen, tmpl, skip = 0; 
   zword_t i, tmpw;
   zbyte_t progress = GOT_NONE; 
   int x, y;

   /* check for IFZS file */
   if ( !read_long( sfp, &tmpl ) || !read_long( sfp, &ifzslen ) )
      return FALSE;
   if ( !read_long( sfp, &currlen ) )
      return FALSE;
   if ( tmpl != ID_FORM || currlen != ID_IFZS )
   {
      output_line( "This is not a saved game file!" );
      return FALSE;
   }
   if ( ( ifzslen & 1 ) || ifzslen < 4 )
      return FALSE;
   ifzslen -= 4;

   /* read a chunk and process it */
   while ( ifzslen > 0 )
   {
      /* read chunk header */
      if ( ifzslen < 8 )
         return FALSE;
      if ( !read_long( sfp, &tmpl ) || !read_long( sfp, &currlen ) )
         return FALSE;
      ifzslen -= 8;

      /* body of chunk */
      if ( ifzslen < currlen )
         return FALSE;
      skip = currlen & 1;
      ifzslen -= currlen + skip;
      switch ( tmpl )
      {
         case ID_IFhd:
            if ( progress & GOT_HEADER )
            {
               output_line( "Save file has two IFhd chunks!" );
               return FALSE;
            }
            progress |= GOT_HEADER;
            if ( currlen < 13 || !read_word( sfp, &i ) )
               return FALSE;
            if ( i != h_version )
               progress = GOT_ERROR;
            for ( i = H_RELEASE_DATE; i < H_RELEASE_DATE + 6; ++i )
            {
               if ( ( x = get_c( sfp ) ) == EOF )
                  return FALSE;
               if ( x != ( int ) get_byte( i ) ) 
                  progress = GOT_ERROR;
            }
            if ( !read_word( sfp, &i ) )
               return FALSE;
            if ( i != h_checksum )
               progress = GOT_ERROR;
            if ( progress == GOT_ERROR )
            {
               output_line( "File was not saved from this story!" );
               return FALSE;
            }
            if ( ( x = get_c( sfp ) ) == EOF )
               return FALSE;
            pc = ( ul_t ) x << 16;
            if ( ( x = get_c( sfp ) ) == EOF )
               return FALSE;
            pc |= ( ul_t ) x << 8;
            if ( ( x = get_c( sfp ) ) == EOF )
               return FALSE;
            pc |= ( ul_t ) x;
            for ( i = 13; ( ul_t ) i < currlen; ++i ) 
               ( void ) get_c( sfp ); /* skip rest of chunk */
            break;
         case ID_Stks:
            if ( progress & GOT_STACK )
            {
               output_line( "File contains two stack chunks!" );
               break;
            }
            progress |= GOT_STACK;
            sp = STACK_SIZE;
            if ( h_type != 6 )
            {
               /* dummy stack frame for stack used before call */
               if ( currlen < 8 )
                  return FALSE;
               for ( i = 0; i < 6; ++i )
                  if ( get_c( sfp ) != 0 )
                     return FALSE;
               if ( !read_word( sfp, &tmpw ) )
                  return FALSE;
               currlen -= 8;
               if ( currlen < (unsigned long)(tmpw * 2) )
                  return FALSE;
               for ( i = 0; i < tmpw; ++i )
                  if ( !read_word( sfp, stack + ( --sp ) ) )
                     return FALSE;
               currlen -= tmpw * 2;
            }
            for ( fp = STACK_SIZE - 1, frame_count = 0; currlen > 0; currlen -= 8 )
            {
               if ( currlen < 8 )
                  return FALSE;
               if ( sp < 4 )
               {
                  output_line( "error: this save-file has too much stack, and I can't cope." );
                  return FALSE;
               }
               /* read PC, procedure flag, and arg count */
               if ( !read_long( sfp, &tmpl ) )
                  return FALSE;
               y = ( zword_t ) tmpl & 0x0F;
               tmpw = y << VAR_SHIFT; /* number of variables */
               /* read result variable  */
               if ( ( x = get_c( sfp ) ) == EOF )
                  return FALSE;

               if ( tmpl & 0x10 )
               {
                  tmpw |= PROCEDURE;
                  tmpl >>= 8;
               }
               else
               {
                  tmpw |= FUNCTION;
                  tmpl >>= 8;
                  --tmpl;
                  /* sanity check on result variable */
                  if ( read_data_byte( &tmpl ) != ( zbyte_t ) x )
                  {
                     output_line( "error: wrong variable number on stack (wrong story file?)." );
                     return FALSE;
                  }
                  --tmpl;       /* read_data_byte increments it */
               }
               stack[--sp] = ( zword_t ) ( tmpl / PAGE_SIZE );
               stack[--sp] = ( zword_t ) ( tmpl % PAGE_SIZE );
               stack[--sp] = fp;

               if ( ( x = get_c( sfp ) ) == EOF )
                  return FALSE;
               ++x;             /* hopefully x now contains a single set bit */

               for ( i = 0; i < 8; ++i )
                  if ( x & ( 1 << i ) )
                     break;
               if ( x ^ ( 1 << i ) ) /* if more than 1 bit set */
               {
                  output_line
                        ( "error: this game uses incomplete argument lists (which I can't handle)." );
                  return FALSE;
               }
               tmpw |= i;
               stack[--sp] = tmpw;
               fp = sp - 1;     /* FP for next frame */
               if ( !read_word( sfp, &tmpw ) )
                  return FALSE;
               tmpw += y;       /* local vars plus eval stack used */
               if ( tmpw >= sp )
               {
                  output_line( "error: this save-file uses more stack than I can cope with." );
                  return FALSE;
               }
               if ( currlen < (unsigned long)(tmpw * 2) )
                  return FALSE;
               for ( i = 0; i < tmpw; ++i )
                  if ( !read_word( sfp, stack + ( --sp ) ) )
                     return FALSE;
               currlen -= tmpw * 2;
            }
            break;
         case ID_ANNO:
            z_buffer_mode( ON );
            for ( ; currlen > 0; --currlen )
               if ( ( x = get_c( sfp ) ) == EOF )
                  return FALSE;
               else
                  write_char( x );
            write_char( ( char ) 13 );
            break;
         case ID_CMem:
            if ( !( progress & GOT_MEMORY ) )
            {
               jz_rewind( gfp );
               i = 0;           /* bytes written to data area */
               for ( ; currlen > 0; --currlen )
               {
                  if ( ( x = get_c( sfp ) ) == EOF )
                     return FALSE;
                  if ( x == 0 ) /* start run */
                  {
                     if ( currlen < 2 )
                     {
                        output_line( "File contains bogus CMem chunk" );
                        for ( ; currlen > 0; --currlen )
                           ( void ) get_c( sfp ); /* skip rest */
                        currlen = 1;
                        i = 0xFFFF;
                        break;  /* keep going in case there's a UMem */
                     }
                     --currlen;
                     if ( ( x = get_c( sfp ) ) == EOF )
                        return FALSE;
                     for ( ; x >= 0 && i < h_restart_size; --x, ++i )
                        if ( ( y = jz_getc( gfp ) ) == EOF )
                           return FALSE;
                        else
                           set_byte( i, y );
                  }
                  else          /* not a run */
                  {
                     if ( ( y = jz_getc( gfp ) ) == EOF )
                        return FALSE;
                     set_byte( i, x ^ y );
                     ++i;
                  }
                  if ( i > h_restart_size )
                  {
                     output_line( "warning: CMem chunk too long!" );
                     for ( ; currlen > 1; --currlen )
                        ( void ) get_c( sfp ); /* skip rest */
                     break;     /* keep going in case there's a UMem */
                  }
               }
               /* if chunk is short, assume a run */
               for ( ; i < h_restart_size; ++i )
                  if ( ( y = jz_getc( gfp ) ) == EOF )
                     return FALSE;
                  else
                     set_byte( i, y );
               if ( currlen == 0 )
                  progress |= GOT_MEMORY; /* only if succeeded */
               break;
            }
            /* Fall thru (to default) if already got memory */
            /* FALLTHROUGH */
         case ID_UMem:
            if ( !( progress & GOT_MEMORY ) )
            {
               if ( currlen == h_restart_size )
               {
                  if ( fread( datap, h_restart_size, 1, sfp ) == 1 )
                  {
                     progress |= GOT_MEMORY; /* only if succeeded */
                     break;
                  }
               }
               else
                  output_line( "warning: UMem chunk wrong size!" );
               /* and fall thru into default */
            }
            /* Fall thru (to default) if already got memory */
            /* FALLTHROUGH */
         default:
            ( void ) fseek( sfp, currlen, SEEK_CUR ); /* skip chunk */
            break;
      }
      if ( skip )
         ( void ) get_c( sfp ); /* skip pad byte */
   }

   if ( !( progress & GOT_HEADER ) )
      output_line( "error: no header chunk in file." );
   if ( !( progress & GOT_STACK ) )
      output_line( "error: no stack chunk in file." );
   if ( !( progress & GOT_MEMORY ) )
      output_line( "error: no memory chunk in file." );
   return ( progress == GOT_ALL );
}


#endif /* defined(USE_QUETZAL) */
