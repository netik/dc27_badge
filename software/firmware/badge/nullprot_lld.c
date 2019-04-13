/*-
 * Copyright (c) 2019
 *      Bill Paul <wpaul@windriver.com>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This module uses the Memory Protection Unit in the Cortex-M4 to implement
 * some basic NULL pointer dereference detection. Trying to read or write
 * though a NULL pointer, e.g. address 0x0 or some close offset from it,
 * is a common coding problem. On most OSes running on application processors,
 * it's possible to trap these errors by reserving page 0 in the program's
 * virtual address space and leaving it unmapped. That way if any code tries
 * to execute a load or store to or from address 0, the MMU will trigger a
 * fault right away.
 * 
 * On Cortex-M devices, you don't have an MMU, and address 0 typically the
 * start of on-board flash memory, as well as the default location of the
 * exception vector table. Performing reads to this location will not
 * trigger a fault, which means that if your code reads through NULL pointer,
 * it will not fail right away, but will instead load some value from the
 * vector table and try to use it as data. This may result in unstable
 * behavior or crashes at some later point which may be difficult to diagnose.
 *
 * The Cortex-M4 doesn't have an MMU, but it does have an MPU, which we
 * can to set access permissions on page to to trap unexpected loads or
 * stores just like an MMU can. Note that while the CPU vector table is
 * also stored at address 0, exception handling doesn't seem to be negatively
 * impacted by the change in access protection.
 */
 
#include "ch.h"
#include "hal.h"

void
nullProtStart (void)
{
	__disable_irq();

	/*
	 * ChibiOS will use MPU region 0 for stack guards. We arbitrarily
	 * choose region 6 for null pointer protection.
	 */

	mpuConfigureRegion (MPU_REGION_6, 0x0, MPU_RASR_ATTR_AP_NA_NA |
	    MPU_RASR_ATTR_NON_CACHEABLE | MPU_RASR_SIZE_1K | MPU_RASR_ENABLE);

	__enable_irq();

	return;
}


void
nullProtStop (void)
{
	__disable_irq();

	mpuConfigureRegion (MPU_REGION_6, 0x0, 0);

	__enable_irq();

	return;
}
