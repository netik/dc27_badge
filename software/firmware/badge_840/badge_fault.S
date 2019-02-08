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

	.cpu		cortex-m4

	.fpu		fpv4-sp-d16

	.thumb

	.file		"badge_fault.S"
	.text

	.section	.text,"ax",%progbits
	.align		1
	.code		16 
	.arch armv7e-m
	.syntax unified
	.thumb
	.thumb_func

	.type		HardFault_Handler, %function
	.type		BusFault_Handler, %function
	.type		UsageFault_Handler, %function
	.type		MemManage_Handler, %function

	.globl		HardFault_Handler
	.globl		BusFault_Handler
	.globl		UsageFault_Handler
	.globl		MemManage_Handler

CommonFault_Handler:
	/*
	 * Test the link register to see if we got here
	 * from thread mode or handler mode. If thread mode,
	 * then switch to using the PSP as the current stack.
	 */
	and	r2, r1, #4
	cmp	r2, #4
	bne	L1
	mrs	r1, psp
	msr	msp, r1
L1:
	mov	r2, sp
	bl	trapHandle

HardFault_Handler:
	mov	r0, #0
	mov	r1, lr
	bl	CommonFault_Handler

BusFault_Handler:
	mov	r0, #1
	mov	r1, lr
	bl	CommonFault_Handler

UsageFault_Handler:
	mov	r0, #2
	mov	r1, lr
	bl	CommonFault_Handler

MemManage_Handler:
	mov	r0, #3
	mov	r1, lr
	bl	CommonFault_Handler
