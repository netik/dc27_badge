/*-
 * Copyright (c) 2018
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

/**
 * @file    hal_qspi_lld.h
 * @brief   NRF52840 QSPI subsystem low level driver header.
 *
 * @addtogroup QSPI
 * @{
 */

#ifndef HAL_QSPI_LLD_H
#define HAL_QSPI_LLD_H

#if (HAL_USE_QSPI == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    QSPI capabilities
 * @{
 */
#define QSPI_SUPPORTS_MEMMAP                TRUE
/** @} */

#define QSPI_CMD_WRITE_STATUS_REGISTER      0x01
#define QSPI_CMD_PAGE_PROGRAM               0x02
#define QSPI_CMD_READ                       0x03
#define QSPI_CMD_READ_STATUS_REGISTER       0x05
#define QSPI_CMD_FAST_READ                  0x0B
#define QSPI_CMD_SUBSECTOR_ERASE            0x20
#define QSPI_CMD_SECTOR_ERASE               0xD8
#define QSPI_CMD_BULK_ERASE		    0xC7
#define QSPI_CMD_READ_FLAG_STATUS_REGISTER  0x70

#define QSPI_FLGSTS_PROG_ERASE	0x80
#define QSPI_FLGSTS_ERASE_SUSP	0x40
#define QSPI_FLGSTS_ERASE_ERR	0x20
#define QSPI_FLGSTS_PRG_ERR	0x10
#define QSPI_FLGSTS_VPP_ERR	0x08
#define QSPI_FLGSTS_PROG_SUSP	0x04
#define QSPI_FLGSTS_PROT_ERR	0x02
#define QSPI_FLGSTS_RSVD	0x01

#define QSPI_STS_STSWREN	0x80
#define QSPI_STS_TOPBOT		0x40
#define QSPI_STS_BLKPROT	0x3C
#define QSPI_STS_WRENLATCH	0x02
#define QSPI_STS_WRBUSY		0x01

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   QSPID1 driver enable switch.
 * @details If set to @p TRUE the support for QSPID1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(NRF5_QSPI_USE_QSPI0) || defined(__DOXYGEN__)
#define NRF5_QSPI_USE_QSPI0             FALSE
#endif
/** @} */

/**
 * @brief   QSPI0 interrupt priority level setting.
 */
#if !defined(NRF5_QSPI_QSPI0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define NRF5_QSPI_QSPI0_IRQ_PRIORITY    3
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a structure representing an QSPI driver.
 */
typedef struct QSPIDriver QSPIDriver;

/**
 * @brief   Type of a QSPI notification callback.
 *
 * @param[in] qspip     pointer to the @p QSPIDriver object triggering the
 *                      callback
 */
typedef void (*qspicallback_t)(QSPIDriver *qspip);

/**
 * @brief   Driver configuration structure.
 */
typedef struct {
  /**
   * @brief   Operation complete callback or @p NULL.
   */
  qspicallback_t             end_cb;
  /* End of the mandatory fields.*/

  uint16_t                   sckpad;
  uint16_t                   csnpad;
  uint16_t                   dio0pad;
  uint16_t                   dio1pad;
  uint16_t                   dio2pad;
  uint16_t                   dio3pad;
  uint32_t                   membase;
} QSPIConfig;

/**
 * @brief   Structure representing an QSPI driver.
 */
struct QSPIDriver {
  /**
   * @brief   Driver state.
   */
  qspistate_t               state;
  /**
   * @brief   Current configuration data.
   */
  const QSPIConfig           *config;
#if (QSPI_USE_WAIT == TRUE) || defined(__DOXYGEN__)
  /**
   * @brief   Waiting thread.
   */
  thread_reference_t        thread;
#endif /* QSPI_USE_WAIT */
#if (QSPI_USE_MUTUAL_EXCLUSION == TRUE) || defined(__DOXYGEN__)
  /**
   * @brief   Mutex protecting the peripheral.
   */
  mutex_t                   mutex;
#endif /* QSPI_USE_MUTUAL_EXCLUSION */
#if defined(QSPI_DRIVER_EXT_FIELDS)
  QSPI_DRIVER_EXT_FIELDS
#endif
  /* End of the mandatory fields.*/

  /**
   * @brief   Pointer to the NRF_QSPI registers block.
   */

  NRF_QSPI_Type *           port;

  uint8_t *                 rxbuf;
  size_t                    rxlen;
  uint8_t                   cmd;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (NRF5_QSPI_USE_QSPI0 == TRUE) && !defined(__DOXYGEN__)
extern QSPIDriver QSPID1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void qspi_lld_init(void);
  void qspi_lld_start(QSPIDriver *qspip);
  void qspi_lld_stop(QSPIDriver *qspip);
  void qspi_lld_command(QSPIDriver *qspip, const qspi_command_t *cmdp);
  void qspi_lld_send(QSPIDriver *qspip, const qspi_command_t *cmdp,
                     size_t n, const uint8_t *txbuf);
  void qspi_lld_receive(QSPIDriver *qspip, const qspi_command_t *cmdp,
                        size_t n, uint8_t *rxbuf);
#if QSPI_SUPPORTS_MEMMAP == TRUE
  void qspi_lld_map_flash(QSPIDriver *qspip,
                          const qspi_command_t *cmdp,
                          uint8_t **addrp);
  void qspi_lld_unmap_flash(QSPIDriver *qspip);
#endif
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_QSPI */

#endif /* HAL_QSPI_LLD_H */

/** @} */
