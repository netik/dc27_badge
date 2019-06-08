#include <stdint.h>
#include <stdlib.h>
#include "crc32.h"

#define     ETHER_CRC_POLY_LE       0xedb88320

uint32_t
crc32_le(const uint8_t *buf, size_t len, uint32_t init)
{
	size_t i;
	uint32_t crc, carry;
	int bit;
	uint8_t data;

	crc = init;       /* initial value */

	for (i = 0; i < len; i++) {
		for (data = *buf++, bit = 0; bit < 8; bit++, data >>= 1) {
			carry = (crc ^ data) & 1;
			crc >>= 1;
			if (carry)
				crc = (crc ^ ETHER_CRC_POLY_LE);
		}
	}

	return (crc);
}
