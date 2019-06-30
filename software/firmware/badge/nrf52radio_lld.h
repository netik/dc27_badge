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

#ifndef _NRF52RADIO_LLD_H_
#define _NRF52RADIO_LLD_H_

/*
 * This is the access address used to send BLE advertisement
 * packets on channels 37, 38 and 39.
 */

#define BLE_ADV_ACCESS_ADDRESS		0x8E89BED6
#define BLE_CHANNELS			40

typedef struct ble_chan {
        uint16_t        ble_freq;
        uint8_t         ble_chan;
} BLE_CHAN;

extern const BLE_CHAN ble_chan_map[BLE_CHANNELS];

extern void nrf52radioStart (void);
extern void nrf52radioStop (void);
extern int nrf52radioRx (uint8_t *, uint8_t, int8_t *, uint32_t);
extern void nrf52radioTx (int);
extern void nrf52radioRxEnable (void);
extern void nrf52radioRxDisable (void);
extern int nrf52radioRssiGet (int8_t *);
extern int nrf52radioChanSet (uint16_t);

#endif /* _NRF52RADIO_LLD_H_ */
