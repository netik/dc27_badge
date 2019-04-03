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

#ifndef _BLE_GATTC_LLD_H_
#define _BLE_GATTC_LLD_H_

#define BLE_GATTC_SERVICE_DISCOVERED		0x00000001
#define BLE_GATTC_CHARACTERISTIC_DISCOVERED	0x00000002
#define BLE_GATTC_DESCRIPTOR_DISCOVERED		0x00000004
#define BLE_GATTC_ATTR_DISCOVERED		0x00000008
#define BLE_GATTC_CHAR_READ			0x00000010
#define BLE_GATTC_CHAR_WRITTEN			0x00000020
#define BLE_GATTC_ERROR				0xFFFFFFFF

extern void bleGattcStart (void);
extern void bleGattcDispatch (ble_evt_t *);
extern int bleGattcRead (uint16_t handle, uint8_t *, uint16_t *, bool);
extern int bleGattcWrite (uint16_t handle, uint8_t *, uint16_t, bool);

#endif /* __BLE_GATTC_LLD_H */
