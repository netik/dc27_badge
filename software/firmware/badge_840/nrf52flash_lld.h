#ifndef NRF52FLASH_LLD_H
#define NRF52FLASH_LLD_H

#include "hal_flash.h"

#define FLASH_PAGE_SIZE 4096

typedef struct {
	int flash_sectors;
} NRF52FLASHConfig;

struct NRF52FLASHDriverVMT {
	_base_flash_methods
};

typedef struct {
	const struct NRF52FLASHDriverVMT *	vmt;
	_base_flash_data
	NRF_NVMC_Type *				port;
	mutex_t					mutex;
	const NRF52FLASHConfig *		config;
} NRF52FLASHDriver;

#endif /* NRF52FLASH_LLD_H */
