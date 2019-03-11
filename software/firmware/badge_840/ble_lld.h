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

#ifndef _BLE_LLD_H_
#define _BLE_LLD_H_

#define BLE_APPEARANCE_DC26	0xDC1B
#define BLE_COMPANY_ID_IDES	0x1DE5
#define BLE_NAME_IDES		"DC27 IDES"
#define BLE_MANUFACTUTER_STRING	"IDES of DEF CON Ltd."
#define BLE_MODEL_NUMBER_STRING	"Revision 1"
#define BLE_SYSTEM_ID_STRING	"DC27 Badge"
#define BLE_SW_VERSION_STRING	__DATE__ "/" __TIME__

#define BLE_IDES_APP_TAG	1

/*
 * These are some GATT service UUIDs defined by the BLE spec for common
 * types of devices. The complete list can be found here:
 * https://www.bluetooth.com/specifications/gatt/services
 */

#define BLE_UUID_DEVICE_INFORMATION_SERVICE	0x180A

/*
 * These are some GATT characteristic UUIDs defined by the BLE spec for
 * different types of services. The complete list can be found here:
 * https://www.bluetooth.com/specifications/gatt/characteristics
 */

#define BLE_UUID_SYSTEM_ID_CHAR			0x2A23
#define BLE_UUID_MODEL_NUMBER_STRING_CHAR	0x2A24
#define BLE_UUID_SOFTWARE_REVISION_STRING_CHAR	0x2A28
#define BLE_UUID_MANUFACTURER_NAME_STRING_CHAR	0x2A29

extern uint8_t ble_station_addr[];
extern volatile enum NRF_SOC_EVTS flash_evt;

extern void bleStart (void);
extern void bleEnable (void);
extern void bleDisable (void);

#endif /* _BLE_LLD_H_ */
