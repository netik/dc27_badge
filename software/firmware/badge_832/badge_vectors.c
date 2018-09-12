#include "ch.h"
#include "hal.h"


/*lint -save -e9075 [8.4] All symbols are invoked from asm context.*/

static void **HARDFAULT_PSP;

void
HardFault_Handler(void)
{
/*lint -restore*/
	/* Hijack the process stack pointer to make backtrace work */

	__asm__("mrs %0, psp" : "=r"(HARDFAULT_PSP) : :);
	__asm__("msr msp, %0" : : "r" (HARDFAULT_PSP) : );

	/* Break into the debugger */
	asm("bkpt #0");

	while(1);
	/* NOTREACHED */
}

/*lint -save -e9075 [8.4] All symbols are invoked from asm context.*/

void
BusFault_Handler(void)
{
/*lint -restore*/
	/* Hijack the process stack pointer to make backtrace work */

	__asm__("mrs %0, psp" : "=r"(HARDFAULT_PSP) : :);
	__asm__("msr msp, %0" : : "r" (HARDFAULT_PSP) : );

	/* Break into the debugger */
	asm("bkpt #0");

	while(1);
	/* NOTREACHED */
}
