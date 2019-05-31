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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <libusb.h>

#include "cp2102.h"

CP2102N_CONFIG config;

static int cp210x_open (uint16_t, uint16_t, libusb_device_handle **);
static void cp210x_close (libusb_device_handle *);
static int cp210x_ee_read (libusb_device_handle *, uint8_t *);
static int cp210x_read (libusb_device_handle *,
    uint16_t, uint8_t *, uint16_t *);
static int cp210x_write (libusb_device_handle *,
    uint16_t, uint8_t *, uint16_t);

static void
cp210x_ascii_to_ucode (char * in, uint16_t * out)
{
	int i;

	for (i = 0; i < strlen(in); i++)
		out[i] = in[i];

	return;
}

static void
cp210x_ucode_to_ascii (uint16_t * in, char * out)
{
	int i = 0;

	while (in[i] != 0x0) {
		out[i] = in[i];
		i++;
	}

	return;
}

static uint16_t
cp210x_csum (uint8_t * data, uint16_t len)
{
	uint16_t sum1 = 0xff, sum2 = 0xff;
	uint16_t tlen;

	while (len) {
		tlen = len >= 20 ? 20 : len;
		len -= tlen;
		do {
			sum2 += sum1 += *data++;
		} while (--tlen);
		sum1 = (sum1 & 0xff) + (sum1 >> 8);
		sum2 = (sum2 & 0xff) + (sum2 >> 8);
	}

	/* Second reduction step to reduce sums to 8 bits */

	sum1 = (sum1 & 0xff) + (sum1 >> 8);
	sum2 = (sum2 & 0xff) + (sum2 >> 8);

	return (sum2 << 8 | sum1);
}

static int
cp210x_open (uint16_t vid, uint16_t did, libusb_device_handle ** d)
{
	struct libusb_device_descriptor desc;
	libusb_context * usbctx = NULL;
	libusb_device_handle * dev = NULL;
	libusb_device ** devlist;
	ssize_t devcnt, i;
	uint8_t strbuf[256];
	int sts = CP210X_ERROR;

	if (libusb_init (&usbctx) != LIBUSB_SUCCESS) {
		fprintf (stderr, "initializing libusb failed\n");
		return (sts);
	}

	/* Get device list */

	devcnt = libusb_get_device_list (usbctx, &devlist);

	if (devcnt < 0) {
		fprintf (stderr, "Getting USB device list failed\n");
		return (sts);
	}

	/* Walk the device in search of a CP210x */

	for (i = 0; i < devcnt; i++) {
		memset (&desc, 0, sizeof (desc));
		libusb_get_device_descriptor (devlist[i], &desc);
		if (desc.idVendor == vid && desc.idProduct == did)
			break;
	}

	if (i == devcnt) {
		fprintf (stderr, "No matching device found for 0x%x/0x%x\n",
		    vid, did);
		*d = NULL;
	} else {
		printf ("Found match for 0x%x/0x%x at bus %d device %d\n",
		    vid, did,  libusb_get_bus_number (devlist[i]),
		    libusb_get_device_address (devlist[i]));
		if (libusb_open (devlist[i], &dev) != LIBUSB_SUCCESS) {
			perror ("Opening device failed");
			*d = NULL;
		} else {
			libusb_get_string_descriptor_ascii (dev,
			    desc.iProduct, strbuf, sizeof (strbuf));
			printf ("Device ID string: [%s]\n", strbuf);
			*d = dev;
			sts = CP210X_OK;
		}
	}

	libusb_free_device_list (devlist, 1);

	return (sts);
}

static void
cp210x_close (libusb_device_handle * d)
{
	libusb_close (d);
	return;
}

static int
cp210x_read (libusb_device_handle * d, uint16_t req,
    uint8_t * buf, uint16_t * len)
{
	int r;
	uint16_t l;

	if (d == NULL || buf == NULL)
		return (CP210X_ERROR);

	l = *len;

	r = libusb_control_transfer (d, LIBUSB_ENDPOINT_IN |
	    LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
	    CP210X_REQUEST_CFG, req, 0, buf, l, CP210X_TIMEOUT_MS);

	if (r < 0) {
		fprintf (stderr, "Reading 0%x failed: %d\n", req, r);
		return (CP210X_ERROR);
	}

	*len = r;

	return (CP210X_OK);
}

static int
cp210x_write (libusb_device_handle * d, uint16_t req,
    uint8_t * buf, uint16_t len)
{
	int r;

	if (d == NULL || buf == NULL)
		return (CP210X_ERROR);

	r = libusb_control_transfer (d, LIBUSB_ENDPOINT_OUT |
	    LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
	    CP210X_REQUEST_CFG, req, 0, buf, len, CP210X_TIMEOUT_MS);

	if (r < 0) {
		fprintf (stderr, "Reading writing: %d\n", r);
		return (CP210X_ERROR);
	}

	return (CP210X_OK);
}

#define PRODUCT_IDES "DC27 IDES of DEF CON"
#define VENDOR_IDES "Team Ides"

int
main (int argc, char * argv[])
{
	int i;
	uint16_t csum;
	uint16_t len;
	char buf[256];
	uint8_t part;
	uint8_t * p;

	libusb_device_handle * dev = NULL;

	if (cp210x_open (CP210X_VENDOR_ID,
	    CP210X_DEVICE_ID, &dev) == CP210X_ERROR) {
		fprintf (stderr, "CP210x not found\n");
		exit (-1);
	}

	part = 0;
	len = 1;
	cp210x_read (dev, CP210X_CFG_MODEL, &part, &len);
	if (part != CP210X_PROD_CP2102N) {
		cp210x_close (dev);
		fprintf (stderr, "device is not a CP2102N (0x%02X)\n", part);
		exit (-1);
	}

	len = sizeof(config);
	cp210x_read (dev, CP210X_CFG_2102N_READ_CONFIG,
	    (uint8_t *)&config, &len);
	memset (buf, 0, sizeof(buf));
	cp210x_ucode_to_ascii ((uint16_t *)config.cp2102n_manstr, buf);
	printf ("Vendor: %s\n", buf);
	memset (buf, 0, sizeof(buf));
	cp210x_ucode_to_ascii ((uint16_t *)config.cp2102n_prodstr, buf);
	printf ("Product: %s\n", buf);

	/* Reset vendor */

	memset (config.cp2102n_manstr, 0,
	    sizeof(config.cp2102n_manstr));
	memset (buf, 0, sizeof(buf));
	csum = (strlen (VENDOR_IDES) + 1) * 2;
	cp210x_ascii_to_ucode (VENDOR_IDES,
	    (uint16_t *)config.cp2102n_manstr);
	cp210x_ucode_to_ascii ((uint16_t *)config.cp2102n_manstr, buf);
	config.cp2102n_manstr_desc.cp2102n_len[0] = csum >> 8;
	config.cp2102n_manstr_desc.cp2102n_len[1] = csum & 0xFF;
	printf ("Vendor: %s\n", buf);

	/* Reset product */

	memset (config.cp2102n_prodstr, 0,
	    sizeof(config.cp2102n_prodstr));
	memset (buf, 0, sizeof(buf));
	csum = (strlen (PRODUCT_IDES) + 1) * 2;
	cp210x_ascii_to_ucode (PRODUCT_IDES,
	    (uint16_t *)config.cp2102n_prodstr);
	cp210x_ucode_to_ascii ((uint16_t *)config.cp2102n_prodstr, buf);
	config.cp2102n_prodstr_desc.cp2102n_len[0] = csum >> 8;
	config.cp2102n_prodstr_desc.cp2102n_len[1] = csum & 0xFF;
	printf ("Product: %s\n", buf);

	/* Update checksum */

	csum = cp210x_csum ((uint8_t *)&config, sizeof(config) - 2);
	config.cp2102n_csum[0] = csum >> 8;
	config.cp2102n_csum[1] = csum & 0xFF;

	len = sizeof(config);
	cp210x_write (dev, CP210X_CFG_2102N_WRITE_CONFIG,
	    (uint8_t *)&config, len);

	cp210x_close (dev);

	exit (0);
}
