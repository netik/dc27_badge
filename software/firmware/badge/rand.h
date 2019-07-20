#ifndef _RAND_H_
#define _RAND_H_

extern void randInit (void);
extern uint8_t randByte (void);
extern uint16_t randUInt16 (void);
extern uint8_t randRange(uint8_t min, uint8_t max);
extern uint16_t randRange16(uint16_t min, uint16_t max);
#endif /* _RAND_H_ */
