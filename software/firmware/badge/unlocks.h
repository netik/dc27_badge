#ifndef __UNLOCKS_H__
#define __UNLOCKS_H__

/* These are a set of bit flags in config->unlocks which can do
 * different things to the badge. There are 11 unlocks. 
 *
 * The number in parenthesis indicates the grant page the buff can be
 * transmitted from. (Hit Up on the fight screen if you have the GOD unlock)
 **/

#define UL_PLUSDEF      ( 1 << 0 ) // EFF + 10% DEF  (1)
#define UL_PLUSHP       ( 1 << 1 ) // +10% HP        (1) - max +10%
#define UL_PLUSMIGHT    ( 1 << 2 ) // +1 MIGHT       (2) - max +1
#define UL_LUCK         ( 1 << 3 ) // +20% LUCK      (1) - max 40%
#define UL_HEAL         ( 1 << 4 ) // 2X HEAL        (1) - max one use
#define UL_LEDS         ( 1 << 5 ) // MORE LEDs      (2) 
#define UL_CAESAR       ( 1 << 6 ) // CAESAR         (2)
#define UL_SENATOR      ( 1 << 7 ) // SENATOR        (2)
#define UL_BENDER       ( 1 << 8 ) // BENDER
#define UL_GOD          ( 1 << 9 ) // GOD_MODE       
#define UL_PINGDUMP     ( 1 << 10 ) // NETWORK DEBUG (BLACK_BADGE only)
#define UL_SIMULATED    ( 1 << 15 ) // PEER is an AI. 
#endif
