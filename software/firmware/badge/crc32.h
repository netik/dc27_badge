#ifndef _CRC32H_
#define _CRC32H_

#define CRC_INIT	0xFFFFFFFF

uint32_t crc32_le(const uint8_t *buf, size_t len, uint32_t init);

#endif /* _CRC32H_ */
