/*-
 * Copyright (c) 2018
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

#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "hal_qspi.h"
#include "hal_flash.h"
#include "m25q.h"
#include "diskio.h"

#define SECTOR_SIZE 512

extern M25QDriver FLASHD1;

static uint8_t * pMem;
static uint32_t disk_size;

DSTATUS
qspi_disk_initialize (void)
{
	const flash_descriptor_t * pFlash;

	pFlash = flashGetDescriptor (&FLASHD1);

	if (pFlash == NULL || pFlash->sectors_count == 0)
		return (RES_ERROR);

	/* Get the disk size */

	disk_size = pFlash->sectors_count * pFlash->sectors_size;

	/* Get the pointer for the XIP mapped region. */

	m25qMemoryUnmap (&FLASHD1);
	m25qMemoryMap (&FLASHD1, &pMem);

	return (RES_OK);
}

DSTATUS
qspi_disk_status (void)
{
	if (disk_size == 0)
        	return (STA_NOINIT);
	return (RES_OK);
}

DRESULT
qspi_disk_read (
        BYTE *buff,	/* Pointer to the data buffer to store read data */
        DWORD sector,		/* Start sector number (LBA) */
        UINT count		/* Sector count (1..128) */
)
{
	uint8_t * p;

	/*
	 * For reads, we can take the shortcut of reading directly from
	 * the XIP mapped area. This saves us having to do a function call.
	 */

	if (pMem == NULL)
		return (RES_ERROR);

	p = pMem + (sector * SECTOR_SIZE);
	memcpy (buff, p, count * SECTOR_SIZE);

	return (RES_OK);
}

DRESULT
qspi_disk_write (
        const BYTE *buff,	/* Pointer to the data to be written */
        DWORD sector,		/* Start sector number (LBA) */
        UINT count		/* Sector count (1..128) */
)
{
	int r;

	m25qMemoryUnmap (&FLASHD1);

	r = flashProgram (&FLASHD1, sector * SECTOR_SIZE,
	    count * SECTOR_SIZE, buff);

	m25qMemoryMap (&FLASHD1, &pMem);

	if (r == FLASH_NO_ERROR)
		return (RES_OK);

	return (RES_ERROR);
}

DRESULT
qspi_disk_ioctl (
        BYTE cmd,		/* Control code */
        void *buff		/* Buffer to send/receive control data */
)
{
	DSTATUS r;
	DWORD * d;
	WORD * w;

	switch (cmd) {
		case CTRL_SYNC:
			r = RES_OK;
			break;
		case GET_SECTOR_COUNT:
			d = buff;
			*d = disk_size / 512;
			r = RES_OK;
			break;
		case GET_SECTOR_SIZE:
			w = buff;
			*w = SECTOR_SIZE;
			r = RES_OK;
			break;
		case GET_BLOCK_SIZE:
			d = buff;
			*d = 1;
			r = RES_OK;
			break;
		default:
			r = RES_PARERR;
			break;
	}

	return (r);
}
