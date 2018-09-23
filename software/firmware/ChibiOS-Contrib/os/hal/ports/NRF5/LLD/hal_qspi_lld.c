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
 * @file    hal_qspi_lld.c
 * @brief   NRF52840 QSPI subsystem low level driver source.
 *
 * @addtogroup QSPI
 * @{
 */

#include "hal.h"

#if (HAL_USE_QSPI == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief QSPID1 driver identifier.*/
#if (NRF5_QSPI_USE_QSPI1 == TRUE) || defined(__DOXYGEN__)
QSPIDriver QSPID1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Shared service routine.
 *
 * @param[in] qspip     pointer to the @p QSPIDriver object
 */
static void qspi_lld_serve_interrupt(QSPIDriver *qspip) {
  NRF_QSPI_Type * port;
  uint32_t d0;
  uint32_t d1;
  size_t cnt;
  size_t i;

  port = qspip->port;

  if (port->EVENTS_READY) {
    /* Ack the interrupt */
    port->EVENTS_READY = 0;  
    (void)port->EVENTS_READY;

    if (qspip->rxbuf != NULL) {

      d0 = port->CINSTRDAT0;
      d1 = port->CINSTRDAT1;

      if (qspip->rxlen > 4)
        cnt = 4;
      else
        cnt = qspip->rxlen;

      for (i = 0; i < cnt; i++) {
        qspip->rxbuf[i] = d0 >> (i * 8);
      }

      if (qspip->rxlen > 4) {
        cnt = qspip->rxlen - 4;
        for (i = 0; i < cnt; i++) {
          qspip->rxbuf[i + 4] = d1 >> (i * 8);
        }
      }

      if (qspip->cmd == QSPI_CMD_READ_FLAG_STATUS_REGISTER)
        qspip->rxbuf[0] = 0x80;

      qspip->rxbuf = NULL;
      qspip->rxlen = 0;
    }

  /* Portable QSPI ISR code defined in the high level driver, note, it is
     a macro.*/
    _qspi_isr_code(qspip);
  }

  return;
}

static void qspi_lld_cmd(QSPIDriver *qspip, uint8_t opcode, uint8_t len)
{
  NRF_QSPI_Type * port;
  volatile uint32_t reg;

  port = qspip->port;

  reg =
    (QSPI_CINSTRCONF_WREN_Enable << QSPI_CINSTRCONF_WREN_Pos) |
    (QSPI_CINSTRCONF_WIPWAIT_Enable << QSPI_CINSTRCONF_WIPWAIT_Pos) |
    (1 << QSPI_CINSTRCONF_LIO2_Pos) |
    (1 << QSPI_CINSTRCONF_LIO3_Pos) |
    (len << QSPI_CINSTRCONF_LENGTH_Pos) |
    (opcode << QSPI_CINSTRCONF_OPCODE_Pos);

  port->CINSTRCONF = reg;

  return;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if NRF5_QSPI_USE_QSPI1 || defined(__DOXYGEN__)
/**
 * @brief   QSPI0 interrupt handler.
 *
 * @isr
 */

CH_IRQ_HANDLER(VectorE4)
{
        CH_IRQ_PROLOGUE();
        qspi_lld_serve_interrupt(&QSPID1);
        CH_IRQ_EPILOGUE();
        return;
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level QSPI driver initialization.
 *
 * @notapi
 */
void qspi_lld_init(void) {

#if NRF5_QSPI_USE_QSPI1
  QSPID1.port = NRF_QSPI;
  qspiObjectInit(&QSPID1);
#endif
}

/**
 * @brief   Configures and activates the QSPI peripheral.
 *
 * @param[in] qspip     pointer to the @p QSPIDriver object
 *
 * @notapi
 */
void qspi_lld_start(QSPIDriver *qspip) {
  NRF_QSPI_Type * port;
  volatile uint32_t reg;

  /* If in stopped state then full initialization.*/
  if (qspip->state == QSPI_STOP) {
#if NRF5_QSPI_USE_QSPI1
    if (&QSPID1 == qspip) {
        nvicEnableVector (QSPI_IRQn, NRF5_QSPI_QSPI3_IRQ_PRIORITY);
    }
#endif

    /* Common initializations.*/
  }

  qspip->rxbuf = NULL;
  qspip->rxlen = 0;

  /* QSPI setup and enable.*/
  port = qspip->port;

  port->PSEL.SCK = qspip->config->sckpad;
  port->PSEL.CSN = qspip->config->csnpad;
  port->PSEL.IO0 = qspip->config->dio0pad;
  port->PSEL.IO1 = qspip->config->dio1pad;
  port->PSEL.IO2 = qspip->config->dio2pad;
  port->PSEL.IO3 = qspip->config->dio3pad;

  reg =  
    (QSPI_IFCONFIG0_READOC_READ4IO << QSPI_IFCONFIG0_READOC_Pos) |
    (QSPI_IFCONFIG0_WRITEOC_PP4IO << QSPI_IFCONFIG0_WRITEOC_Pos) |
    (QSPI_IFCONFIG0_DPMENABLE_Disable << QSPI_IFCONFIG0_DPMENABLE_Pos) |
    (QSPI_IFCONFIG0_ADDRMODE_24BIT << QSPI_IFCONFIG0_ADDRMODE_Pos) |
    (QSPI_IFCONFIG0_PPSIZE_256Bytes << QSPI_IFCONFIG0_PPSIZE_Pos);
  port->IFCONFIG0 = reg;

  /* Using 0 for the SCKFREQ divisor should get us a clock of 32MHz. */

  reg = port->IFCONFIG1;
  reg &= 0x00FFFF00;
  reg |=
    (0 << QSPI_IFCONFIG1_SCKFREQ_Pos) |
    (QSPI_IFCONFIG1_DPMEN_Exit << QSPI_IFCONFIG1_DPMEN_Pos) |
    (QSPI_IFCONFIG1_SPIMODE_MODE0 << QSPI_IFCONFIG1_SPIMODE_Pos) |
    (1 << QSPI_IFCONFIG1_SCKDELAY_Pos);
  port->IFCONFIG1 = reg;

  port->ENABLE = QSPI_ENABLE_ENABLE_Msk;

  port->EVENTS_READY = 0;
  (void)port->EVENTS_READY;

  port->INTENSET = QSPI_INTENSET_READY_Msk;
  (void)port->INTENSET;

  osalSysLock ();
  port->TASKS_ACTIVATE = QSPI_TASKS_ACTIVATE_TASKS_ACTIVATE_Msk;
  (void) osalThreadSuspendS(&qspip->thread);
  osalSysUnlock ();

  return;
}

/**
 * @brief   Deactivates the QSPI peripheral.
 *
 * @param[in] qspip     pointer to the @p QSPIDriver object
 *
 * @notapi
 */
void qspi_lld_stop(QSPIDriver *qspip) {
  NRF_QSPI_Type * port;

  /* If in ready state then disables QSPI.*/
  if (qspip->state == QSPI_READY) {

    /* QSPI disable.*/

    port = qspip->port;
    port->ENABLE = 0;

    /* Stopping involved clocks.*/
#if NRF5_QSPI_USE_QSPI1
    if (&QSPID1 == qspip) {
        nvicDisableVector (QSPI_IRQn);
    }
#endif

  }
}

/**
 * @brief   Sends a command without data phase.
 * @post    At the end of the operation the configured callback is invoked.
 *
 * @param[in] qspip     pointer to the @p QSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 *
 * @notapi
 */
void qspi_lld_command(QSPIDriver *qspip, const qspi_command_t *cmdp) {
  NRF_QSPI_Type * port;

  qspip->cmd = cmdp->cfg & QSPI_CFG_CMD_MASK;

  port = qspip->port;

  if ((cmdp->cfg & QSPI_CFG_CMD_MASK) == QSPI_CMD_SECTOR_ERASE) {
    port->ERASE.PTR = cmdp->addr;
    port->ERASE.LEN = QSPI_ERASE_LEN_LEN_64KB;
    port->TASKS_ERASESTART = QSPI_TASKS_ERASESTART_TASKS_ERASESTART_Msk;
    return;
  }

  if ((cmdp->cfg & QSPI_CFG_CMD_MASK) == QSPI_CMD_SUBSECTOR_ERASE) {
    port->ERASE.PTR = cmdp->addr;
    port->ERASE.LEN = QSPI_ERASE_LEN_LEN_4KB;
    port->TASKS_ERASESTART = QSPI_TASKS_ERASESTART_TASKS_ERASESTART_Msk;
    return;
  }

  if ((cmdp->cfg & QSPI_CFG_CMD_MASK) == QSPI_CMD_BULK_ERASE) {
    port->ERASE.PTR = cmdp->addr;
    port->ERASE.LEN = QSPI_ERASE_LEN_LEN_All;
    port->TASKS_ERASESTART = QSPI_TASKS_ERASESTART_TASKS_ERASESTART_Msk;
    return;
  }

  qspi_lld_cmd (qspip, cmdp->cfg & QSPI_CFG_CMD_MASK,
    QSPI_CINSTRCONF_LENGTH_1B);

  return;
}

/**
 * @brief   Sends a command with data over the QSPI bus.
 * @post    At the end of the operation the configured callback is invoked.
 *
 * @param[in] qspip     pointer to the @p QSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 * @param[in] n         number of bytes to send
 * @param[in] txbuf     the pointer to the transmit buffer
 *
 * @notapi
 */
void qspi_lld_send(QSPIDriver *qspip, const qspi_command_t *cmdp,
                   size_t n, const uint8_t *txbuf) {
  NRF_QSPI_Type * port;
  uint32_t d0;
  uint32_t d1;
  size_t cnt;
  size_t i;

  port = qspip->port;

  qspip->cmd = cmdp->cfg & QSPI_CFG_CMD_MASK;

  /*
   * Ok, here's the deal: the qspiSend() and qspiReceive() APIs have been
   * overloaded to handle both commands _and_ raw data. I suspect that this
   * reflects the interface of the QSPI controller in the ST Micro part that
   * was originally used to prototype QSPI support in ChibiOS.
   *
   * Rather than requiring you to use raw command transactions to handle
   * plain flash reads and writes, the Nordic QSPI controller has a shortcut
   * registers. These can be used to specify exactly what addresses and
   * lengths to read/write, and the controller will issue the necessary
   * QSPI commands internally.
   *
   * When asked to do a bare data write here, the addr field in the command
   * descriptor will be non-zero. If we see that, then we issue a write via
   * the shortcut registers instead of trying to issue a bare command.
   */

  if ((cmdp->cfg & QSPI_CFG_CMD_MASK) == QSPI_CMD_PAGE_PROGRAM) {
    port->WRITE.DST = cmdp->addr;
    port->WRITE.SRC = (uint32_t)txbuf;
    port->WRITE.CNT = n;
    port->TASKS_WRITESTART = QSPI_TASKS_WRITESTART_TASKS_WRITESTART_Msk;
    return;
  }

  if (n > 8)
    return;

  d0 = 0;
  d1 = 0;

  if (n > 4)
     cnt = 4;
  else
     cnt = n;

  for (i = 0; i < cnt; i++) {
    d0 |= txbuf[i] << (i * 8);
  }

  if (n > 4) {
     cnt = n - 4;
    for (i = 0; i < cnt; i++) {
      d1 |= txbuf[i + 4] << (i * 8);
    }
  }

  port->CINSTRDAT0 = d0;
  port->CINSTRDAT1 = d1;

  qspi_lld_cmd (qspip, cmdp->cfg & QSPI_CFG_CMD_MASK,
    QSPI_CINSTRCONF_LENGTH_1B + n);

  return;
}

/**
 * @brief   Sends a command then receives data over the QSPI bus.
 * @post    At the end of the operation the configured callback is invoked.
 *
 * @param[in] qspip     pointer to the @p QSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 * @param[in] n         number of bytes to send
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void qspi_lld_receive(QSPIDriver *qspip, const qspi_command_t *cmdp,
                      size_t n, uint8_t *rxbuf) {
  NRF_QSPI_Type * port;

  port = qspip->port;

  qspip->cmd = cmdp->cfg & QSPI_CFG_CMD_MASK;

  if ((cmdp->cfg & QSPI_CFG_CMD_MASK) == QSPI_CMD_READ) {
    port->READ.SRC = cmdp->addr;
    port->READ.DST = (uint32_t)rxbuf;
    port->READ.CNT = n;
    port->TASKS_READSTART = QSPI_TASKS_READSTART_TASKS_READSTART_Msk;
    return;
  }

  if (n > 8)
    n = 8;

  port->CINSTRDAT0 = 0;
  port->CINSTRDAT1 = 0;
  qspip->rxbuf = rxbuf;
  qspip->rxlen = n;

  qspi_lld_cmd (qspip, cmdp->cfg & QSPI_CFG_CMD_MASK,
    QSPI_CINSTRCONF_LENGTH_1B + n);

  (void)rxbuf;
}

#if (QSPI_SUPPORTS_MEMMAP == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Maps in memory space a QSPI flash device.
 * @pre     The memory flash device must be initialized appropriately
 *          before mapping it in memory space.
 *
 * @param[in] qspip     pointer to the @p QSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 * @param[out] addrp    pointer to the memory start address of the mapped
 *                      flash or @p NULL
 *
 * @notapi
 */
void qspi_lld_map_flash(QSPIDriver *qspip,
                        const qspi_command_t *cmdp,
                        uint8_t **addrp) {
  NRF_QSPI_Type * port;

  (void)cmdp;

  port = qspip->port;

  qspi_lld_cmd (qspip, cmdp->cfg & QSPI_CFG_CMD_MASK,
    QSPI_CINSTRCONF_LENGTH_1B);

  port->XIPOFFSET = qspip->config->membase;

  *addrp = (uint8_t *)qspip->config->membase;

  return;
}

/**
 * @brief   Unmaps from memory space a QSPI flash device.
 * @post    The memory flash device must be re-initialized for normal
 *          commands exchange.
 *
 * @param[in] qspip     pointer to the @p QSPIDriver object
 *
 * @notapi
 */
void qspi_lld_unmap_flash(QSPIDriver *qspip) {
  NRF_QSPI_Type * port;

  port = qspip->port;

  qspi_lld_stop (qspip);
  port->XIPOFFSET = 0;
  qspi_lld_start (qspip);

  return;
}
#endif /* QSPI_SUPPORTS_MEMMAP == TRUE */

#endif /* HAL_USE_QSPI */

/** @} */
