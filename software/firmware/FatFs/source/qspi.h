/*-----------------------------------------------------------------------
/  Low level disk interface modlue include file   (C)ChaN, 2016
/-----------------------------------------------------------------------*/

#ifndef _QSPI_DEFINED
#define _QSPI_DEFINED

#include "diskio.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------*/
/* Prototypes for disk control functions */

DSTATUS qspi_disk_initialize (void);
DSTATUS qspi_disk_status (void);
DRESULT qspi_disk_read (BYTE* buff, DWORD sector, UINT count);
DRESULT qspi_disk_write (const BYTE* buff, DWORD sector, UINT count);
DRESULT qspi_disk_ioctl (BYTE cmd, void* buff);
void qspi_disk_timerproc (void);

#ifdef __cplusplus
}
#endif

#endif
