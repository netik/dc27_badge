#ifndef _NRF_H_
#define _NRF_H_

/*
 * This is a dummy placeholder file. Some of the SoftDevice headers expect
 * to include an nrf.h file from the Nordic SDK. We're not using this
 * yet, but we need something for the files to #include to make things
 * build, and I'd prefer to keep the original SoftDevice files intact
 * instead of butchering them to match our setup.
 */


/*
 * These are for compatibility with SoftDevice 5.1.0. They're tied to
 * the macro names in the nrf52_bitfields.h file which Nordic ships with
 * their SDK. They assume that everybody will use the SoftDevice only
 * with their SDK, and thus will sometimes make changes like this and hope
 * nobody notices. Well I noticed.
 */

#define SWI1_IRQn		SWI1_EGU2_IRQn
#define SWI1_IRQHandler		SWI1_EGU2_IRQHandler

#define SWI2_IRQn		SWI2_EGU2_IRQn
#define SWI2_IRQHandler		SWI2_EGU2_IRQHandler

#endif /* _NRF_H_ */
