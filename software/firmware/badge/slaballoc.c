#include <stdio.h>
#include <string.h>
#include "slaballoc.h"
#include "ch.h"

/* tiny allocator for transmit buffer memory */
TXBUF  slabs[MAX_SLABS];

void sl_init(void) {
  int i;
  for (i=0; i < MAX_SLABS;i++) {
    slabs[i].active = FALSE;
    memset(slabs[i].txb_data, 0, sizeof(slabs[i].txb_data));
  }
}

uint8_t *sl_alloc(void) {
  int i;

  for (i=0; i < MAX_SLABS;i++) {
    if (slabs[i].active == FALSE) {
      memset(slabs[i].txb_data, 0, sizeof(slabs[i].txb_data));
      slabs[i].active = TRUE;
      //printf("slalloc = %p\n", (void *) &(slabs[i].txb_data[0]));
      return(&(slabs[i].txb_data[0]));
    }
  }

  /* we failed to find memory */
  return NULL;
}

void sl_release(uint8_t *p) {
  /* we expect to receive a pointer here from the tx complete event */
  /* if we go back one byte we'll be pointing to the start of the struct */
  TXBUF *t;

  t = (TXBUF *)(p-1);
  //printf("slrelease = %p\n", (void *) t);
  t->active = FALSE;
}
