
#ifndef __UNLOCKS_H__
#define __UNLOCKS_H__

/* These are a set of bit flags in config->unlocks which can do
 * different things to the badge. There are 11 unlocks.
 *
 * The number in parenthesis indicates the grant page the buff can be
 * transmitted from. (Hit Up on the fight screen if you have the GOD unlock)
 **/

#define MAX_ULCODES 11
#define MAX_CODE_LEN 5

/*
 * Note: the UL_BLACKBADGE bit is not unlockable by the user using
 * the app-unlock puzzle app. It's outside the range of MAX_ULCODES,
 * which is all the puzzle app supports. The only ways to enable it are:
 * 1) add it to the default config->unlocks word at compile time
 * 2) unlock it via radio using a phone, with the access password
 * 3) unlock it using the special app-radiounlock app (which also
 * uses the access password)
 */

#define UL_PLUSDEF      ( 1 << 0 )  // +10% DEF - ok
#define UL_PLUSHP       ( 1 << 1 )  // +10% HP - ok
#define UL_PLUSDMG      ( 1 << 2 )  // +10% DMG - ok
#define UL_SPEED        ( 1 << 3 )  // +20% SPEED - not done
#define UL_REPAIR       ( 1 << 4 )  // FAST REPAIR - ok`
#define UL_LEDS         ( 1 << 5 )  // MORE LEDs - ok
#define UL_VIDEO1       ( 1 << 6 )  // Unlocks additional videos - ok
#define UL_SHIP1        ( 1 << 7 )  // Unlocks additional ships - ok
#define UL_SHIP2        ( 1 << 8 )  // Unlocks additional ship #2 - ok
#define UL_GOD          ( 1 << 9 )  // GOD_MODE - not done
#define UL_PINGDUMP     ( 1 << 10 ) // NETWORK DEBUG (BLACK_BADGE only)
#define UL_BLACKBADGE	  ( 1 << 11 ) // black badge

/* we store our codes in the UICR instead of in files */
/* check the provisioning script to change the codes. */
#define UL_CODE_0  (&NRF_UICR->CUSTOMER[20])
#define UL_CODE_1  (&NRF_UICR->CUSTOMER[21])
#define UL_CODE_2  (&NRF_UICR->CUSTOMER[22])
#define UL_CODE_3  (&NRF_UICR->CUSTOMER[23])
#define UL_CODE_4  (&NRF_UICR->CUSTOMER[24])
#define UL_CODE_5  (&NRF_UICR->CUSTOMER[25])
#define UL_CODE_6  (&NRF_UICR->CUSTOMER[26])
#define UL_CODE_7  (&NRF_UICR->CUSTOMER[27])
#define UL_CODE_8  (&NRF_UICR->CUSTOMER[28])
#define UL_CODE_9  (&NRF_UICR->CUSTOMER[29])
#define UL_CODE_10 (&NRF_UICR->CUSTOMER[30])

#define UL_PUZPIN_KEY	((((uint64_t)NRF_UICR->CUSTOMER[19]) << 16) | \
			    (NRF_UICR->CUSTOMER[18] >> 16))
#define UL_PUZMODE_PIN	(&NRF_UICR->CUSTOMER[31])

#endif
