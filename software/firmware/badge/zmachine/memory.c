
/* $Id: memory.c,v 1.2 2000/05/25 22:28:56 jholder Exp $
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information
 * --------------------------------------------------------------------
 *
 * File name: $Id: memory.c,v 1.2 2000/05/25 22:28:56 jholder Exp $
 *
 * Description:
 *
 * Modification history:
 * $Log: memory.c,v $
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
 * memory.c
 *
 * Code and data caching routines
 *
 */

#include "ztypes.h"

/* A cache entry */

typedef struct cache_entry
{
   struct cache_entry *flink;
   int page_number;
   zbyte_t data[PAGE_SIZE];
}
cache_entry_t;

/* Cache chain anchor */

static cache_entry_t *cache = NULL;

/* Pseudo translation buffer, one entry each for code and data pages */

static unsigned int current_code_page = 0;
static cache_entry_t *current_code_cachep = NULL;
static unsigned int current_data_page = 0;
static cache_entry_t *current_data_cachep = NULL;

static unsigned int calc_data_pages( void );
static cache_entry_t *update_cache( int );

/*
 * load_cache
 *
 * Initialise the cache and any other dynamic memory objects. The memory
 * required can be split into two areas. Firstly, three buffers are required for
 * input, output and status line. Secondly, two data areas are required for
 * writeable data and read only data. The writeable data is the first chunk of
 * the file and is put into non-paged cache. The read only data is the remainder
 * of the file which can be paged into the cache as required. Writeable data has
 * to be memory resident because it cannot be written out to a backing store.
 *
 */

void load_cache( void )
{
   unsigned long file_size;
   unsigned int i, file_pages, data_pages;
   cache_entry_t *cachep;

   /* Allocate output and status line buffers */

   line = ( char * ) malloc( screen_cols + 1 );
   if ( line == NULL )
   {
      fatal( "load_cache(): Insufficient memory to play game" );
   }
   status_line = ( char * ) malloc( screen_cols + 1 );
   if ( status_line == NULL )
   {
      fatal( "load_cache(): Insufficient memory to play game" );
   }

   /* Must have at least one cache page for memory calculation */
   cachep = ( cache_entry_t * ) malloc( sizeof ( cache_entry_t ) );

   if ( cachep == NULL )
   {
      fatal( "load_cache(): Insufficient memory to play game" );
   }
   cachep->flink = cache;
   cachep->page_number = 0;
   cache = cachep;

   /* Calculate dynamic cache pages required */

   if ( h_config & CONFIG_MAX_DATA )
   {
      data_pages = calc_data_pages(  );
   }
   else
   {
      data_pages = ( h_data_size + PAGE_MASK ) >> PAGE_SHIFT;
      //printf("h_data_size: ");printf(h_data_size);printf(", PAGE_MASK: ");printf(PAGE_MASK);
   }
   data_size = data_pages * PAGE_SIZE;
   file_size = ( unsigned long ) h_file_size *story_scaler;

   file_pages = ( unsigned int ) ( ( file_size + PAGE_MASK ) >> PAGE_SHIFT );
   //printf("data pages: ");printf(data_pages);printf(", file pages: ");printf(file_pages);
   //printf("data size: "); printf(data_size);printf(", file size: "); printf(file_size);
   /* Allocate static data area and initialise it */

   datap = ( zbyte_t * ) malloc( data_size );
   if ( datap == NULL )
   {
      fatal( "load_cache(): Insufficient memory to play game" );
   }
   for ( i = 0; i < data_pages; i++ )
   {
      read_page( i, &datap[i * PAGE_SIZE] );
   }

   /* Allocate memory for undo */

   undo_datap = ( zbyte_t * ) malloc( data_size );

   /* Allocate cache pages and initialise them */

   for ( i = data_pages; cachep != NULL && i < file_pages; i++ )
   {
      cachep = ( cache_entry_t * ) malloc( sizeof ( cache_entry_t ) );

      if ( cachep != NULL )
      {
         cachep->flink = cache;
         cachep->page_number = i;
         //printf("loading cache page ");printf(i);printf(", link: ");printf((uint32_t) cachep->flink,HEX);
         read_page( cachep->page_number, cachep->data );
         cache = cachep;
      }
   }

}                               /* load_cache */

/*
 * unload_cache
 *
 * Deallocate cache and other memory objects.
 *
 */

void unload_cache( void )
{
   cache_entry_t *cachep, *nextp;

   /* Make sure all output has been flushed */

   z_new_line(  );

   /* Free output buffer, status line and data memory */

   free( line );
   free( status_line );
   free( datap );
   free( undo_datap );

   /* Free cache memory */

   for ( cachep = cache; cachep->flink != NULL; cachep = nextp )
   {
      nextp = cachep->flink;
      free( cachep );
   }
   cache = NULL; // needed by Arduino to make games repeatable
}                               /* unload_cache */

/*
 * read_code_word
 *
 * Read a word from the instruction stream.
 *
 */

zword_t read_code_word( void )
{
   zword_t w;

   w = ( zword_t ) read_code_byte(  ) << 8;
   w |= ( zword_t ) read_code_byte(  );

   return ( w );

}                               /* read_code_word */

/*
 * read_code_byte
 *
 * Read a byte from the instruction stream.
 *
 */

zbyte_t read_code_byte( void )
{
   unsigned int page_number, page_offset;
   /* Calculate page and offset values */

   page_number = ( unsigned int ) ( pc >> PAGE_SHIFT );
   page_offset = ( unsigned int ) pc & PAGE_MASK;
   //printf("read_code_byte(): ");printf("reading page ");printf(page_number);printf(", offset ");printf(page_offset);
   /* Load page into translation buffer */

   //if ( page_number != current_code_page )
   if ( page_number != current_code_page || current_code_cachep == NULL)

   {
     current_code_cachep = update_cache( page_number );
     current_code_page = page_number;
   }

   /* Update the PC */

   pc++;
   /* Return byte from page offset */

   if ( !current_code_cachep )
   {
      fatal
            ( "read_code_byte(): read from non-existant page!\n\t(Your dynamic memory usage _may_ be over 64k in size!)" );
   }

   return ( current_code_cachep->data[page_offset] );

}                               /* read_code_byte */

zbyte_t read_code_byte_old( void )
{
   unsigned int page_number, page_offset;

   /* Calculate page and offset values */

   page_number = ( unsigned int ) ( pc >> PAGE_SHIFT );
   page_offset = ( unsigned int ) pc & PAGE_MASK;

   /* Load page into translation buffer */

   if ( page_number != current_code_page )
   {
      current_code_cachep = update_cache( page_number );
      current_code_page = page_number;
   }

   /* Update the PC */

   pc++;

   /* Return byte from page offset */

   if ( !current_code_cachep )
   {
      fatal
            ( "read_code_byte(): read from non-existant page!\n\t(Your dynamic memory usage _may_ be over 64k in size!)" );
   }

   return ( current_code_cachep->data[page_offset] );

}                               /* read_code_byte */

/*
 * read_data_word
 *
 * Read a word from the data area.
 *
 */

zword_t read_data_word( unsigned long *addr )
{
   zword_t w;

   w = ( zword_t ) read_data_byte( addr ) << 8;
   w |= ( zword_t ) read_data_byte( addr );

   return ( w );

}                               /* read_data_word */

/*
 * read_data_byte
 *
 * Read a byte from the data area.
 *
 */

zbyte_t read_data_byte( unsigned long *addr )
{
   unsigned int page_number, page_offset;
   zbyte_t value;

   /* Check if byte is in non-paged cache */

   if ( *addr < ( unsigned long ) data_size )
   {
      value = datap[*addr];
   }
   else
   {
      /* Calculate page and offset values */

      page_number = ( int ) ( *addr >> PAGE_SHIFT );
      page_offset = ( int ) *addr & PAGE_MASK;

      /* Load page into translation buffer */

      if ( page_number != current_data_page )
      {
         current_data_cachep = update_cache( page_number );
         current_data_page = page_number;
      }

      /* Fetch byte from page offset */

      if ( current_data_cachep )
      {
         value = current_data_cachep->data[page_offset];
      }
      else
      {
         fatal( "read_data_byte(): Fetching data from invalid page!" );
      }
   }

   /* Update the address */

   ( *addr )++;

   return ( value );

}                               /* read_data_byte */

/*
 * calc_data_pages
 *
 * Compute the best size for the data area cache. Some games have the data size
 * header parameter set too low. This causes a write outside of data area on
 * some games. To alleviate this problem the data area size is set to the
 * maximum of the restart size, the data size and the end of the dictionary. An
 * attempt is made to put the dictionary in the data area to stop paging during
 * a dictionary lookup. Some games have the dictionary end very close to the
 * 64K limit which may cause problems for machines that allocate memory in
 * 64K chunks.
 *
 */

static unsigned int calc_data_pages( void )
{
   unsigned long offset, data_end, dictionary_end;
   int separator_count, word_size, word_count;
   unsigned int data_pages;

   /* Calculate end of data area, use restart size if data size is too low */

   if ( h_data_size > h_restart_size )
   {
      data_end = h_data_size;
   }
   else
   {
      data_end = h_restart_size;
   }

   /* Calculate end of dictionary table */

   offset = h_words_offset;
   separator_count = read_data_byte( &offset );
   offset += separator_count;
   word_size = read_data_byte( &offset );
   word_count = read_data_word( &offset );
   dictionary_end = offset + ( word_size * word_count );

   /* If data end is too low then use end of dictionary instead */

   if ( dictionary_end > data_end )
   {
      data_pages = ( unsigned int ) ( ( dictionary_end + PAGE_MASK ) >> PAGE_SHIFT );
   }
   else
   {
      data_pages = ( unsigned int ) ( ( data_end + PAGE_MASK ) >> PAGE_SHIFT );
   }

   return ( data_pages );

}                               /* calc_data_pages */

/*
 * update_cache
 *
 * Called on a code or data page cache miss to find the page in the cache or
 * read the page in from disk. The chain is kept as a simple LRU chain. If a
 * page cannot be found then the page on the end of the chain is reused. If the
 * page is found, or reused, then it is moved to the front of the chain.
 *"
 */

static cache_entry_t *update_cache( int page_number )
{
   //printf("update_cache(");printf(page_number); printf(")");
   cache_entry_t *cachep, *lastp;

   /* Search the cache chain for the page */

   for ( lastp = cache, cachep = cache;
         cachep->flink != NULL && cachep->page_number && cachep->page_number != page_number;
         lastp = cachep, cachep = cachep->flink )
         {
          //printf("next page: "); printf(cachep->page_number); printf(", ptr: ");printf((uint32_t) cachep->flink,HEX);
          if((uint32_t) cachep->page_number > 2000)
          {
            fatal("Bad page!");
          }
         }
      //;

   /* If no page in chain then read it from disk */

   if ( cachep->page_number != page_number )
   {
      /* Reusing last cache page, so invalidate cache if page was in use */
      if ( cachep->flink == NULL && cachep->page_number )
      {
         if ( current_code_page == ( unsigned int ) cachep->page_number )
         {
            current_code_page = 0;
         }
         if ( current_data_page == ( unsigned int ) cachep->page_number )
         {
            current_data_page = 0;
         }
      }

      /* Load the new page number and the page contents from disk */
      //printf("load new page ");printf(page_number);
      cachep->page_number = page_number;
      read_page( page_number, cachep->data );
   }

   /* If page is not at front of cache chain then move it there */

   if ( lastp != cache )
   {
      lastp->flink = cachep->flink;
      cachep->flink = cache;
      cache = cachep;
   }

   return ( cachep );

}                               /* update_cache */
