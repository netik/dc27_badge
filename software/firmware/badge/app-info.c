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

#include "orchard-app.h"
#include "orchard-ui.h"

#include "buildtime.h"
#include "nrf_sdm.h"

#include "ff.h"
#include "ffconf.h"
#include "diskio.h"

#include "nrf52i2s_lld.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

extern uint32_t __ram7_init_text__;
extern char   __heap_base__; /* Set by linker */
extern char   __heap_end__; /* Set by linker */

#pragma pack(1)
typedef struct _sd_cid {
	uint8_t		sd_mid;		/* Manufacturer ID */
	char		sd_oid[2];	/* OEM/Application ID */
	char		sd_pnm[5];	/* Product name */
	uint8_t		sd_prv;		/* Product revision */
	uint32_t	sd_psn;		/* Product serial number */
	uint16_t	sd_mdt;		/* Manufacture data */
	uint8_t		sd_crc;		/* 8-bit CRC */
} sd_cid;
#pragma pack()

typedef struct _ihandles {
	GListener	gl;
	GHandle		ghConsole;
} IHandles;

static uint32_t
info_init (OrchardAppContext *context)
{
	(void)context;

	/*
	 * We don't want any extra stack space allocated for us.
	 * We'll use the heap.
	 */

	return (0);
}

static void
info_start (OrchardAppContext *context)
{
	GSourceHandle gs;
        GWidgetInit wi;
	IHandles * p;
	uint32_t info;
	uint8_t * v;
	struct mallinfo m;
	uint64_t disksz;
	DWORD sectors;
	BYTE drv;
	sd_cid cid;

	/*
	 * Play a jaunty tune to keep the user entertained.
	 * (ARE YOU NOT ENTERTAINED?!)
	 */

	i2sWait ();
	i2sPlay ("sound/cantina.snd");

	p = malloc (sizeof(IHandles));

	context->priv = p;

	gs = ginputGetMouse (0);
	geventListenerInit (&p->gl);
	geventAttachSource (&p->gl, gs, GLISTEN_MOUSEMETA);
	geventRegisterCallback (&p->gl, orchardAppUgfxCallback, &p->gl);

	/* Create a console widget */

	gwinWidgetClearInit (&wi);
	wi.g.show = FALSE;
	wi.g.x = 0;
	wi.g.y = 0;
	wi.g.width = gdispGetWidth();
	wi.g.height = gdispGetHeight();
	p->ghConsole = gwinConsoleCreate (0, &wi.g);
	gwinSetColor (p->ghConsole, White);
	gwinSetBgColor (p->ghConsole, Blue);
	gwinShow (p->ghConsole);
	gwinClear (p->ghConsole);

	/* Now display interesting info. */

	/* SoC info */

	info = NRF_FICR->INFO.VARIANT;
	v = (uint8_t *)&info;
	gwinPrintf (p->ghConsole, "SoC: Nordic Semiconductor nRF%lx "
	    "Variant: %c%c%c%c\n", NRF_FICR->INFO.PART,
	    v[3], v[2], v[1], v[0]);
	gwinPrintf (p->ghConsole, "RAM: %ldKB Flash: %ldKB Package: ",
	    NRF_FICR->INFO.RAM, NRF_FICR->INFO.FLASH);
	switch (NRF_FICR->INFO.PACKAGE) {
		case FICR_INFO_PACKAGE_PACKAGE_QI:
			gwinPrintf (p->ghConsole, "73-pin aQFN");
			break;
		case 0x2005:
			gwinPrintf (p->ghConsole, "WLCSP");
			break;
		default:
			gwinPrintf (p->ghConsole, "unknown");
			break;
	}
	gwinPrintf (p->ghConsole, "\n");
	gwinPrintf (p->ghConsole, "Device ID: %lX%lX\n",
	    NRF_FICR->DEVICEID[0], NRF_FICR->DEVICEID[1]);
	v = (uint8_t *)&NRF_FICR->DEVICEADDR[0];
	gwinPrintf (p->ghConsole, "BLE station address: %X:%X:%X:%X:%X:%X\n",
	    v[5], v[4], v[3], v[2], v[1], v[0]);

	/* CPU core info */
 
	gwinPrintf (p->ghConsole, "CPU Core: ARM (0x%x) Cortex-M4 (0x%x) "
	    "version r%dp%d\n",
	    (SCB->CPUID & SCB_CPUID_IMPLEMENTER_Msk) >>
	      SCB_CPUID_IMPLEMENTER_Pos,
	    (SCB->CPUID & SCB_CPUID_PARTNO_Msk) >>
	      SCB_CPUID_PARTNO_Pos,
	    (SCB->CPUID & SCB_CPUID_VARIANT_Msk) >>
	      SCB_CPUID_VARIANT_Pos,
	    (SCB->CPUID & SCB_CPUID_REVISION_Msk) >>
	      SCB_CPUID_REVISION_Pos);
	gwinPrintf (p->ghConsole, "Debugger: %s\n",
	    CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk ?
	    "connected" : "disconnected");

	/* Firmware info */

	gwinPrintf (p->ghConsole, "ChibiOS kernel version: %s\n",
	    CH_KERNEL_VERSION);
	gwinPrintf (p->ghConsole, "SoftDevice S140 version: %d.%d.%d\n",
	    SD_MAJOR_VERSION, SD_MINOR_VERSION, SD_BUGFIX_VERSION);
	gwinPrintf (p->ghConsole, "Firmware image size: %d bytes\n",
	    (uint32_t)&__ram7_init_text__ - 2);
	gwinPrintf (p->ghConsole, "%s\n", BUILDTIME);
	gwinPrintf (p->ghConsole, "Built with: %s\n", PORT_COMPILER_NAME);

	/* Memory info */

	m = mallinfo ();
	gwinPrintf (p->ghConsole, "Heap size: %d bytes  ",
	    &__heap_end__ - &__heap_base__);
	gwinPrintf (p->ghConsole, "Arena size: %d bytes\n", m.arena);
	gwinPrintf (p->ghConsole, "Arena in use: %d bytes  ", m.uordblks);
	gwinPrintf (p->ghConsole, "Arena free: %d bytes\n", m.fordblks);

	/* Storage info */

	drv = 0;
	gwinPrintf (p->ghConsole, "SD card: ");
	if (disk_ioctl (drv, GET_SECTOR_COUNT, &sectors) == RES_OK &&
	    disk_ioctl (drv, MMC_GET_CID, &cid) == RES_OK) {
		disksz = sectors * 512ULL;
		disksz /= 1048576ULL;
		gwinPrintf (p->ghConsole, "%d MB ", disksz);
		gwinPrintf (p->ghConsole, "[%c%c] ", cid.sd_oid[0],
		    cid.sd_oid[1]);
		gwinPrintf (p->ghConsole, "[%c%c%c%c%c] ", cid.sd_pnm[0],
		    cid.sd_pnm[1], cid.sd_pnm[2], cid.sd_pnm[3],
		    cid.sd_pnm[4]);
		gwinPrintf (p->ghConsole, "SN: %lx\n", cid.sd_psn);
	} else
		gwinPrintf (p->ghConsole, "not found\n");

	return;
}

static void
info_event (OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	GEventMouse * me;

	(void)event;
	(void)context;

	if (event->type == keyEvent && event->key.flags == keyPress) {
		if (event->key.code == keyASelect ||
		    event->key.code == keyBSelect) {
			i2sPlay ("sound/click.snd");
			orchardAppExit ();
			return;
		}
	}

	if (event->type == ugfxEvent) {
		me = (GEventMouse *)event->ugfx.pEvent;
		if (me->buttons & GMETA_MOUSE_DOWN) {
			i2sPlay ("sound/click.snd");
			orchardAppExit ();
			return;
		}
	}

	return;
}

static void
info_exit (OrchardAppContext *context)
{
	IHandles * p;

	p = context->priv;

	geventRegisterCallback (&p->gl, NULL, NULL);
	geventDetachSource (&p->gl, NULL);

	gwinDestroy (p->ghConsole);

	free (p);

	return;
}

orchard_app("Badge Info", "icons/search.rgb",
    0, info_init, info_start, info_event, info_exit, 9999);
