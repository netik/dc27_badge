/*-
 * Copyright (c) 2016
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

#ifndef _XPT2046_REG_H_
#define _XPT2046_REG_H_


#define XPT_CTL_PD0		0x01	/* Power down select 0 */
#define XPT_CTL_PD1		0x02	/* Power down select 1 */
#define XPT_CTL_SDREF		0x04	/* Single ended/differential refsel */
#define XPT_CTL_MODE		0x08	/* Conversion mode */
#define XPT_CTL_CHAN		0x70	/* Channel select */
#define XPT_CTL_START		0x80	/* Start bit */

#define XPT_PD_POWERDOWN	0x00	/* Power down between conversions */
#define XPT_PD_REFOFF_ADCON	0x01
#define XPT_PD_REFON_ADCOFF	0x02
#define XPT_PD_POWERUP		0x03	/* Always powered */

#define XPT_MODE_8BITS		0x08
#define XPT_MODE_12BITS		0x00

#define XPT_CHAN_TEMP0		0x00
#define XPT_CHAN_Y		0x10	/* Y position */
#define XPT_CHAN_VBAT		0x20	/* Battery voltage */
#define XPT_CHAN_Z1		0x30	/* Z1 position */
#define XPT_CHAN_Z2		0x40	/* Z2 position */
#define XPT_CHAN_X		0x50	/* X position */
#define XPT_CHAN_AUX		0x60	/* Aux input */
#define XPT_CHAN_TEMP1		0x70	/* Temperature */

#endif /* _XPT2046_REG_H_ */
