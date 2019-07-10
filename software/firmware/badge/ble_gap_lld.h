/*-
 * Copyright (c) 2017
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

#ifndef _BLE_GAP_LLD_H_
#define _BLE_GAP_LLD_H_

extern uint16_t ble_conn_handle;
extern uint8_t ble_gap_role;
extern ble_gap_addr_t ble_peer_addr;

/*
 * SoftDevice 6 supports extended advertisements. The macro
 * BLE_GAP_ADV_MAX_SIZE was removed and replaced with a different
 * one. We add a compatibility definition here.
 */

#ifndef BLE_GAP_ADV_MAX_SIZE
#define BLE_GAP_ADV_MAX_SIZE BLE_GAP_ADV_SET_DATA_SIZE_MAX
#endif

#define BLE_IDES_SCAN_TIMEOUT		100
#define BLE_IDES_ADV_TIMEOUT		100
#define BLE_IDES_CONNECT_TIMEOUT	500

enum
{
    UNIT_0_625_MS = 625,        /**< Number of microseconds in 0.625 milliseconds. */
    UNIT_1_25_MS  = 1250,       /**< Number of microseconds in 1.25 milliseconds. */
    UNIT_10_MS    = 10000       /**< Number of microseconds in 10 milliseconds. */
};


#define MSEC_TO_UNITS(TIME, RESOLUTION) (((TIME) * 1000) / (RESOLUTION))

extern void bleGapDispatch (ble_evt_t *);

extern void bleGapStart (void);

extern int bleGapConnect (ble_gap_addr_t *);
extern int bleGapDisconnect (void);

extern void bleGapUpdateState (uint16_t x, uint16_t y, uint16_t xp,
    uint8_t rank, uint8_t type,  bool in_combat);

extern void bleGapUpdateName (void);

uint32_t bleGapAdvBlockFind (uint8_t **, uint8_t *, uint8_t);

#endif /* __BLE_GAP_LLD_H */
