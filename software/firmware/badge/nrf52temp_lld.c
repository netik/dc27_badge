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

#include "ch.h"
#include "hal.h"
#include "osal.h"

#include "nrf_soc.h"
#include "nrf_sdm.h"

#include "nrf52temp_lld.h"

static volatile int32_t nrf52_temp;
static thread_reference_t tempThreadReference;

OSAL_IRQ_HANDLER(Vector70)
{
	OSAL_IRQ_PROLOGUE();

	osalSysLockFromISR ();
	nrf52_temp = NRF_TEMP->TEMP;

	/*
	 * Workaround for PAN_028 rev2.0A anomaly 28 - TEMP:
	 * Negative measured values are not represented correctly
	 */

	if (nrf52_temp & MASK_SIGN)
		nrf52_temp |= MASK_SIGN_EXTENSION;

	NRF_TEMP->EVENTS_DATARDY = 0;
	(void)NRF_TEMP->EVENTS_DATARDY;

	NRF_TEMP->TASKS_STOP = 1;

	osalThreadResumeI (&tempThreadReference, MSG_OK);
	osalSysUnlockFromISR ();

	OSAL_IRQ_EPILOGUE();
	return;
}

void
nrf52TempStart (void)
{
	volatile uint32_t * pTempConfig = (uint32_t *)(NRF_TEMP_BASE + 0x504);

	/*
	 * Workaround for Errata 66 "TEMP: Linearity specification
	 * not met with default settings"
	 * Load the temperature configuration registers from the
	 * values specified in the FICR region.
	 */

	NRF_TEMP->A0 = NRF_FICR->TEMP.A0;
	NRF_TEMP->A1 = NRF_FICR->TEMP.A1;
	NRF_TEMP->A2 = NRF_FICR->TEMP.A2;
	NRF_TEMP->A3 = NRF_FICR->TEMP.A3;
	NRF_TEMP->A4 = NRF_FICR->TEMP.A4;
	NRF_TEMP->A5 = NRF_FICR->TEMP.A5;
	NRF_TEMP->B0 = NRF_FICR->TEMP.B0;
	NRF_TEMP->B1 = NRF_FICR->TEMP.B1;
	NRF_TEMP->B2 = NRF_FICR->TEMP.B2;
	NRF_TEMP->B3 = NRF_FICR->TEMP.B3;
	NRF_TEMP->B4 = NRF_FICR->TEMP.B4;
	NRF_TEMP->B5 = NRF_FICR->TEMP.B5;
	NRF_TEMP->T0 = NRF_FICR->TEMP.T0;
	NRF_TEMP->T1 = NRF_FICR->TEMP.T1;
	NRF_TEMP->T2 = NRF_FICR->TEMP.T2;
	NRF_TEMP->T3 = NRF_FICR->TEMP.T3;
	NRF_TEMP->T4 = NRF_FICR->TEMP.T4;

	/*
	 * Workaround for PAN_028 rev2.0A anomaly 31 - TEMP:
	 * Temperature offset value has to be manually
	 * loaded to the TEMP module
	 */

	*pTempConfig = 0;

	NRF_TEMP->INTENSET = TEMP_INTENSET_DATARDY_Msk;

	nvicEnableVector (TEMP_IRQn, NRF5_TEMP_IRQ_PRIORITY);

	return;
}

void
nrf52TempStop (void)
{
	NRF_TEMP->INTENCLR = TEMP_INTENCLR_DATARDY_Msk;

	nvicDisableVector (TEMP_IRQn);

	return;
}

int
nrf52TempGet (int32_t * t)
{
	osalSysLock ();
	NRF_TEMP->TASKS_START = 1;

	(void) osalThreadSuspendS (&tempThreadReference);

	osalSysUnlock ();

	*t = nrf52_temp;

	return (NRF_SUCCESS);
}

int
tempGet (int32_t * t)
{
	uint8_t sdenabled;
	int r;

	rngAcquireUnit (&RNGD1);

	sd_softdevice_is_enabled (&sdenabled);

	if (sdenabled)
		r = sd_temp_get (t);
	else
		r = nrf52TempGet (t);

	rngReleaseUnit (&RNGD1);

	return (r);
}

