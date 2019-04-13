#ifndef _RAND_H_
#define _RAND_H_

extern void randInit (void);
extern uint8_t randByte (void);
extern uint8_t randRange(uint8_t min, uint8_t max);

#endif /* _RAND_H_ */
