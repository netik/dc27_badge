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

#include <stdio.h>

/*
 * Information on fault handling comes from the Cortex-M4 Generic User Guide
 * http://infocenter.arm.com/help/topic/com.arm.doc.dui0553b/DUI0553.pdf
 */

#define HARD_FAULT	0
#define BUS_FAULT	1
#define USAGE_FAULT	2
#define MEMMANAGE_FAULT	3

/*
 * Cortex-M4 exception frame, including floating point state.
 */

typedef struct exc_frame {
	uint32_t		exc_R[4];
	uint32_t		exc_R12;
	uint32_t		exc_LR;
	uint32_t		exc_PC;
	uint32_t		exc_xPSR;
	uint32_t		exc_fpregs[16];
	uint32_t		exc_FPSCR;
	uint32_t		exc_dummy;
} EXC_FRAME;

/*
 * Cortex-M4 exception types.
 */

static char * exc_msg[] = {
	"Instruction access violation",
	"Data access violation",
	NULL,
	"Memory management fault on unstacking during exception return",
	"Memory management fault on stacking during exception entry",
	"Memory management fault during floating point context save",
	NULL,
	NULL,
	"Instruction bus error",
	"Precise data bus error",
	"Imprecise data bus error",
	"Bus error on unstacking during exception return",
	"Bus error on stacking during exception entry",
	"Bus error during floating point context save",
	NULL,
	NULL,
	"Illegal instruction",
	"Invalid EPSR state",
	"Invalid PC load via EXC_RETURN",
	"Unsupported coprocessor",
	NULL,
	NULL,
	NULL,
	NULL,
	"Unaligned access trap",
	"Division by zero",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static char exc_msgbuf[80];

static void
_putc (char c)
{
	NRF_UART0->TXD = (uint32_t)c;
	(void)NRF_UART0->TXD;
	while (NRF_UART0->EVENTS_TXDRDY == 0)
		__DSB();
	NRF_UART0->EVENTS_TXDRDY = 0;
	(void)NRF_UART0->EVENTS_TXDRDY;

	return;
}

static void
_puts (char * str)
{
	char * p;
	p = str;
	while (*p != '\0') {
		_putc (*p);
		p++;
	}
	_putc ('\r');
	_putc ('\n');
	return;
}

static void
dumpFrame (int type, uint32_t lr, EXC_FRAME *p)
{
	int i;

	for (i = 0; i < 32; i++) {
		if (SCB->CFSR & (1 << i) && exc_msg[i] != NULL)
			_puts (exc_msg[i]);
	}

	if (SCB->HFSR & SCB_HFSR_VECTTBL_Msk)
		_puts ("Bus fault on vector table read during exception");

	if (type == HARD_FAULT && SCB->HFSR & SCB_HFSR_FORCED_Msk)
		_puts ("Forced fault due to configurable priority violation");

	if (SCB->CFSR & SCB_CFSR_MMARVALID_Msk) {
		sprintf (exc_msgbuf, "Memory fault fault address: 0x%08lX",
		    SCB->BFAR);
		_puts (exc_msgbuf);
	}

	if (SCB->CFSR & SCB_CFSR_BFARVALID_Msk) {
		sprintf (exc_msgbuf, "Bus fault address: 0x%08lX",
		    SCB->BFAR);
		_puts (exc_msgbuf);
	}

	sprintf (exc_msgbuf, "Fault while in %s mode", lr & 0x8 ?
	    "thread" : "handler");
	_puts (exc_msgbuf);

	sprintf (exc_msgbuf, "Floating point context %ssaved on stack",
	    lr & 0x10 ? "not " : "");
	_puts (exc_msgbuf);

	if (SCB->ICSR & SCB_ICSR_VECTPENDING_Msk) {
		sprintf (exc_msgbuf, "Interrupt is pending");
		_puts (exc_msgbuf);
	}

	if (SCB->ICSR & SCB_ICSR_VECTPENDING_Msk) {
		sprintf (exc_msgbuf, "Exception pending: %ld",
		    (SCB->ICSR & SCB_ICSR_VECTPENDING_Msk) >>
		    SCB_ICSR_VECTPENDING_Pos);
		_puts (exc_msgbuf);
	}

	if (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) {
		sprintf (exc_msgbuf, "Exception active: %ld",
		    (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) >>
		    SCB_ICSR_VECTACTIVE_Pos);
		_puts (exc_msgbuf);
	}

	sprintf (exc_msgbuf, "PC: 0x%08lX LR: 0x%08lX "
	    "SP: 0x%08lX SR: 0x%08lX",
	    p->exc_PC, p->exc_LR,
	    (uint32_t)p + (lr & 0x10 ? 32 : sizeof(EXC_FRAME)),
	    p->exc_xPSR);
	_puts (exc_msgbuf);
	sprintf (exc_msgbuf, "R0: 0x%08lX R1: 0x%08lX "
	    "R2: 0x%08lX R3: 0x%08lX R12: 0x%08lX",
	    p->exc_R[0], p->exc_R[1], p->exc_R[2], p->exc_R[3], p->exc_R12);
	_puts (exc_msgbuf);

	return;
}

void
trapHandle (int type, uint32_t exc_lr, EXC_FRAME * exc_sp)
{
	/* Reset the serial port. */

	NRF_UART0->INTENCLR = 0xFFFFFFFF;
	NRF_UART0->TASKS_STOPTX = 1;
	NRF_UART0->TASKS_STARTTX = 1;

	/* Drain any pending TX events */

	while (NRF_UART0->EVENTS_TXDRDY != 0) {
		NRF_UART0->EVENTS_TXDRDY = 0;
		(void)NRF_UART0->EVENTS_TXDRDY;
	}

	_puts ("");
	_puts ("");

	switch (type) {
		case HARD_FAULT:
			_puts ("********** HARD FAULT **********");
			dumpFrame (type, exc_lr, exc_sp);
			break;
		case BUS_FAULT:
			_puts ("********** BUS FAULT **********");
			dumpFrame (type, exc_lr, exc_sp);
			break;
		case USAGE_FAULT:
			_puts ("********** USAGE FAULT **********");
			dumpFrame (type, exc_lr, exc_sp);
			break;
		case MEMMANAGE_FAULT:
			_puts ("********** MEMMANAGE FAULT **********");
			dumpFrame (type, exc_lr, exc_sp);
			break;
		default:
			_puts ("********** unknown fault **********");
			break;
	}

	/* Break into the debugger */

	__asm__ ("bkpt #0");

	while (1) {
	}

	/* NOTREACHED */
}
