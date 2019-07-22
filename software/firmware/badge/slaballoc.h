#ifndef _SLABALLOC_H_
#define _SLABALLOC_H_
#define MAX_SLABS 16

typedef struct _txbuf {
   uint8_t     active;
   uint8_t     txb_data[64];
} TXBUF;

extern void sl_release(uint8_t *p);
extern void sl_init(void);
extern uint8_t *sl_alloc(void);
extern TXBUF  slabs[MAX_SLABS];

#endif /* _SLABALLOC_H_ */
