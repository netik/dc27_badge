/*-
 * Copyright (c) 2018, 2019
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
 * This module provides support for accessing the NRF52840's internal flash
 * using the ChibiOS flash API. We support two access modes: direct NVMC
 * access and SoftDevice access. Since we use the SoftDevice for BLE
 * connectivity, SoftDevice mode is typically used by default (the user
 * is required to use SoftDevice mode whenever it's enabled.
 */

#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "nrf52flash_lld.h"

#if (HAL_USE_SOFTDEVICE == TRUE)
#include "nrf_soc.h"
#include "ble_lld.h"
#endif

static const flash_descriptor_t *nrf52_get_descriptor(void *instance);
static flash_error_t nrf52_read(void *instance, flash_offset_t offset,
                               size_t n, uint8_t *rp);
static flash_error_t nrf52_program(void *instance, flash_offset_t offset,
                                  size_t n, const uint8_t *pp);
static flash_error_t nrf52_start_erase_all(void *instance);
static flash_error_t nrf52_start_erase_sector(void *instance,
                                             flash_sector_t sector);
static flash_error_t nrf52_query_erase(void *instance, uint32_t *msec);
static flash_error_t nrf52_verify_erase(void *instance, flash_sector_t sector);

static const struct NRF52FLASHDriverVMT nrf52_vmt = {
	(size_t)0,		/* instance offset */
	nrf52_get_descriptor,
	nrf52_read,
	nrf52_program,
	nrf52_start_erase_all,
	nrf52_start_erase_sector,
	nrf52_query_erase,
	nrf52_verify_erase
};

static flash_descriptor_t nrf52_descriptor = {
	0,			/* attributes */
	FLASH_PAGE_SIZE,	/* page_size */
	256U,			/* sectors_count - nRF52840 has 1MB flash */
	NULL,			/* sectors */
	FLASH_PAGE_SIZE,	/* sectors_size */
	0U			/* address */
};

static const flash_descriptor_t *
nrf52_get_descriptor (void *instance)
{
	NRF52FLASHDriver *devp;

	devp = (NRF52FLASHDriver *)instance;

	if (devp->state == FLASH_UNINIT ||
	    devp->state == FLASH_STOP)
		return (NULL);

	return (&nrf52_descriptor);
}

static flash_error_t
nrf52_read (void *instance, flash_offset_t offset, size_t n, uint8_t *rp)
{
	NRF52FLASHDriver *devp = (NRF52FLASHDriver *)instance;
	uint8_t * p;

	if ((offset + (n * FLASH_PAGE_SIZE)) >
	    (FLASH_PAGE_SIZE * nrf52_descriptor.sectors_count))
		return (FLASH_ERROR_READ);

	osalMutexLock (&devp->mutex);

	if (devp->state != FLASH_READY) {
		osalMutexUnlock (&devp->mutex);
		return (FLASH_ERROR_PROGRAM);
	}

	p = (uint8_t *)nrf52_descriptor.address;
	memcpy (rp, p + offset, n);

	osalMutexUnlock (&devp->mutex);

	return (FLASH_NO_ERROR);
}

static flash_error_t
nrf52_start_erase_sector (void *instance, flash_sector_t sector)
{
	NRF52FLASHDriver *devp = (NRF52FLASHDriver *)instance;
#if (HAL_USE_SOFTDEVICE == TRUE)
	int r;
#else
	flash_offset_t offset = (flash_offset_t)(sector * FLASH_PAGE_SIZE);
#endif

	if (sector > (nrf52_descriptor.sectors_count - 1))
		return (FLASH_ERROR_ERASE);

	osalMutexLock (&devp->mutex);

	if (devp->state == FLASH_ERASE) {
		osalMutexUnlock (&devp->mutex);
		return (FLASH_BUSY_ERASING);
	}

	devp->state = FLASH_ERASE;

#if (HAL_USE_SOFTDEVICE == TRUE)
	flash_evt = 0;
	r = sd_flash_page_erase (sector);
#else
	devp->port->CONFIG = NVMC_CONFIG_WEN_Een;
        devp->port->ERASEPAGE = offset;
#endif

	osalMutexUnlock (&devp->mutex);

#if (HAL_USE_SOFTDEVICE == TRUE)
	if (r == NRF_SUCCESS)
		return (FLASH_NO_ERROR);

	return (FLASH_ERROR_ERASE);
#else
	return (FLASH_NO_ERROR);
#endif
}

static flash_error_t
nrf52_query_erase (void *instance, uint32_t *msec)
{
	NRF52FLASHDriver *devp = (NRF52FLASHDriver *)instance;
#if (HAL_USE_SOFTDEVICE == TRUE)
	enum NRF_SOC_EVTS evt;
#endif

	osalMutexLock (&devp->mutex);

	if (devp->state != FLASH_ERASE) {
		osalMutexUnlock (&devp->mutex);
		return (FLASH_ERROR_PROGRAM);
	}

#if (HAL_USE_SOFTDEVICE == TRUE)
	if (flash_evt == 0) {
		if (msec != NULL)
			*msec = 1U;
		osalMutexUnlock (&devp->mutex);
		return (FLASH_BUSY_ERASING);
	}
	evt = flash_evt;
	flash_evt = 0;
	if (NRF_EVT_FLASH_OPERATION_SUCCESS)
		devp->state = FLASH_READY;
#else
	if (devp->port->READY == NVMC_READY_READY_Busy) {
		if (msec != NULL)
			*msec = 1U;
		osalMutexUnlock (&devp->mutex);
		return (FLASH_BUSY_ERASING);
	}

        devp->port->CONFIG = NVMC_CONFIG_WEN_Ren;
	devp->state = FLASH_READY;
#endif
	osalMutexUnlock (&devp->mutex);

#if (HAL_USE_SOFTDEVICE == TRUE)
	if (evt == NRF_EVT_FLASH_OPERATION_SUCCESS)
		return (FLASH_NO_ERROR);
	return (FLASH_ERROR_ERASE);
#else
	return (FLASH_NO_ERROR);
#endif
}

static flash_error_t
nrf52_verify_erase (void *instance, flash_sector_t sector)
{
	(void)instance;
	(void)sector;

	return (FLASH_ERROR_ERASE);
}

static flash_error_t
nrf52_start_erase_all(void *instance)
{
	(void)instance;
	return (FLASH_ERROR_ERASE);
}

static flash_error_t
nrf52_program (void *instance, flash_offset_t offset,
               size_t n, const uint8_t *pp)
{
	NRF52FLASHDriver *devp = (NRF52FLASHDriver *)instance;
	uint8_t * p;
#if (HAL_USE_SOFTDEVICE == TRUE)
	enum NRF_SOC_EVTS evt;
	int r;
#else
	uint32_t * s;
	uint32_t * d;
	unsigned i;
#endif

	/*
         * Check that the size is always a multiple of 4 bytes, since
	 * the flash must be written in words.
	 */

	if (n & 0x3)
		return (FLASH_ERROR_PROGRAM);

	if ((offset + n) > (FLASH_PAGE_SIZE * nrf52_descriptor.sectors_count))
		return (FLASH_ERROR_PROGRAM);

	osalMutexLock (&devp->mutex);

	if (devp->state != FLASH_READY) {
		osalMutexUnlock (&devp->mutex);
		return (FLASH_ERROR_PROGRAM);
	}

	p = (uint8_t *)nrf52_descriptor.address;

#if (HAL_USE_SOFTDEVICE == TRUE)
	flash_evt = 0;
	r = sd_flash_write ((uint32_t *)(p + offset),
	    (const uint32_t *)pp, n / sizeof(uint32_t));
	if (r != NRF_SUCCESS) {
		osalMutexUnlock (&devp->mutex);
		return (FLASH_ERROR_PROGRAM);
	}
	while (flash_evt == 0)
		__WFE();
	evt = flash_evt;
	flash_evt = 0;
#else
	s = (uint32_t *)pp;
	d = (uint32_t *)(p + offset);

	devp->port->CONFIG = NVMC_CONFIG_WEN_Wen;

	for (i = 0; i < (n / sizeof(uint32_t)); i++)
		d[i] = s[i];

	while (devp->port->READY == NVMC_READY_READY_Busy)
                ;
	devp->port->CONFIG = NVMC_CONFIG_WEN_Ren;
#endif

	osalMutexUnlock (&devp->mutex);

#if (HAL_USE_SOFTDEVICE == TRUE)
	if (evt == NRF_EVT_FLASH_OPERATION_SUCCESS)
		return (FLASH_NO_ERROR);

	return (FLASH_ERROR_PROGRAM);
#else
	return (FLASH_NO_ERROR);
#endif
}

void
nrf52FlashObjectInit(NRF52FLASHDriver *devp)
{
	devp->vmt = &nrf52_vmt;
	devp->state = FLASH_STOP;
	devp->config = NULL;
	devp->port = NRF_NVMC;
	osalMutexObjectInit (&devp->mutex);

	return;
}

void
nrf52FlashStart(NRF52FLASHDriver *devp, const NRF52FLASHConfig *config)
{
	devp->config = config;

	nrf52_descriptor.sectors_count = config->flash_sectors;

        while (devp->port->READY == NVMC_READY_READY_Busy)
                ;

	devp->state = FLASH_READY;

	return;
}

void
nrf52FlashStop(NRF52FLASHDriver *devp)
{
	devp->config = NULL;

	return;
}

