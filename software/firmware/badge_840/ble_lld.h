/*-
 * Copyright (c) 2017-2019
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

#define BLE_IDES_PASSWORD	"narf"
#define BLE_APPEARANCE_DC27	0xDC1B
#define BLE_COMPANY_ID_IDES	0x1DE5
#define BLE_NAME_IDES		"DC27 IDES of DEF CON"
#define BLE_MANUFACTUTER_STRING	"IDES of DEF CON Ltd."
#define BLE_MODEL_NUMBER_STRING	"Revision 1"
#define BLE_SYSTEM_ID_STRING	"DC27 Badge"
#define BLE_SW_VERSION_STRING	__DATE__ "/" __TIME__

#define BLE_IDES_APP_TAG	1

/*
 * Our private Badge service UUID. Note that private UUIDs work in kind of
 * a funny way. First, for whatever the reason, the Nordic stack sends them
 * out in reverse order (from last byte to first). I don't know why: 128 bit
 * UUIDs are essentially byte arrays, which don't have any ordering rules.
 *
 * Second, with a custom service, the service and all characteristics
 * therein get their own unique 128-bit UUID, but they're all derived
 * from the vendor-specific base UUID. The base UUID is 128 bits long,
 * but two octets, at offsets 2 and 3, are reserved and contain the
 * service and characteristic values. In the Nordic software, these are
 * specified as 16-bit values, but they get folded into bytes 2 and 3
 * of the base UUID.
 *
 * We leave these bytes as 0 in our vendor UUID since they'll be filled
 * in later.
 */

#define BLE_IDES_UUID_BASE					\
	{ 0x11, 0x09, 0x76, 0xED, 0xAE, 0xB8, 0x2D, 0xAC,	\
	  0xED, 0x11, 0x7D, 0x52, 0x00, 0x00, 0x24, 0xAA }
#define BLE_IDES_UUID_STRING	"aa24xxxx-527d-11e9-ac2d-b8aeed760911"

/*
 * These are some GATT service UUIDs defined by the BLE spec for common
 * types of devices. The complete list can be found here:
 * https://www.bluetooth.com/specifications/gatt/services
 */

#define BLE_UUID_GENERIC_ACCESS_SERVICE		0x1800
#define BLE_UUID_GENERIC_ATTRIBUTE_SERVICE	0x1801
#define BLE_UUID_DEVICE_INFORMATION_SERVICE	0x180A

/*
 * These are some GATT declaration UUIDs defined by the BLE spec for
 * different types of declarations. These can be used to enumerate
 * GATT services provided by a peripheral device. The complete list can
 * be found here: https://www.bluetooth.com/specifications/gatt/declarations
 */

#define BLE_UUID_PRIMARY_SERVICE_DECLARATION	0x2800
#define BLE_UUID_SECONDARY_SERVICE_DECLARATION	0x2801
#define BLE_UUID_INCLUDE_DECLARATION		0x2802
#define BLE_UUID_CHARACTERISTIC_DECLARATION	0x2803

/*
 * These are some GATT characteristic UUIDs defined by the BLE spec for
 * different types of services. The complete list can be found here:
 * https://www.bluetooth.com/specifications/gatt/characteristics
 */

#define BLE_UUID_SYSTEM_ID_CHAR			0x2A23
#define BLE_UUID_MODEL_NUMBER_STRING_CHAR	0x2A24
#define BLE_UUID_SOFTWARE_REVISION_STRING_CHAR	0x2A28
#define BLE_UUID_MANUFACTURER_NAME_STRING_CHAR	0x2A29

/*
 * BLE role types. For some reason these are not defined in the
 * SoftDevice headers.
 */

#define BLE_ADV_ROLE_PERIPH_ONLY		0
#define BLE_ADV_ROLE_CENTRAL_ONLY		1
#define BLE_ADV_ROLE_BOTH_PERIPH_PREFERRED	2
#define BLE_ADV_ROLE_BOTH_CENTRAL_PREFERRED	3

/*
 * These are our custom UUIDs for our private service
 */

#define BLE_UUID_IDES_BADGE_SERVICE		0x0000


#define BLE_UUID_IDES_BADGE_PASSWORD		0x0001
#define BLE_UUID_IDES_BADGE_UNLOCKS		0x0002
#define BLE_UUID_IDES_BADGE_CHATREQUEST		0x0003
#define BLE_UUID_IDES_BADGE_GAMEVAL1		0x0004

extern uint32_t ble_unlocks;
extern uint32_t ble_chatreq;
extern char ble_password[32];
extern uint8_t ble_station_addr[];
extern volatile uint32_t flash_evt;

extern void bleStart (void);
extern void bleEnable (void);
extern void bleDisable (void);

#endif /* _BLE_LLD_H_ */
