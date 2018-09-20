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

/**
 * @file    NRF5/LLD/hal_spim_lld.c
 * @brief   NRF5 low level SPI master with DMA driver code.
 *
 * @addtogroup SPI
 * @{
 */

#include "hal.h"

#if HAL_USE_SPI || defined(__DOXYGEN__)

#if NRF5_SPI_USE_DMA == TRUE || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if NRF5_SPI_USE_SPI0 || defined(__DOXYGEN__)
/** @brief SPI1 driver identifier.*/
SPIDriver SPID1;
#endif

#if NRF5_SPI_USE_SPI1 || defined(__DOXYGEN__)
/** @brief SPI2 driver identifier.*/
SPIDriver SPID2;
#endif

#if NRF5_SPI_USE_SPI2 || defined(__DOXYGEN__)
/** @brief SPI3 driver identifier.*/
SPIDriver SPID3;
#endif

#if NRF5_SPI_USE_SPI3 || defined(__DOXYGEN__)
/** @brief SPI4 driver identifier.*/
SPIDriver SPID4;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Preloads the transmit FIFO.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 */

static void port_fifo_preload(SPIDriver *spip)
{
	NRF_SPIM_Type *port = spip->port;

	port->TXD.PTR = (uint32_t)spip->txptr;
	port->RXD.PTR = (uint32_t)spip->rxptr;

	if (spip->txcnt > NRF5_SPIM_DMA_CHUNK)
		port->TXD.MAXCNT = NRF5_SPIM_DMA_CHUNK;
	else
		port->TXD.MAXCNT = spip->txcnt;

	if (spip->rxcnt > NRF5_SPIM_DMA_CHUNK)
		port->RXD.MAXCNT = NRF5_SPIM_DMA_CHUNK;
	else
		port->RXD.MAXCNT = spip->rxcnt;

	return;
}

#if defined(__GNUC__)
__attribute__((noinline))
#endif
/**
 * @brief   Common IRQ handler.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 */
static void serve_interrupt(SPIDriver *spip)
{
	NRF_SPIM_Type *port = spip->port;

	if (port->EVENTS_END) {
		/* Ack the interrupt */
		port->EVENTS_END = 0;  
		(void)port->EVENTS_END;

		/* Accumulate transfered data */
		spip->txcnt -= port->TXD.AMOUNT;
		spip->rxcnt -= port->RXD.AMOUNT;

#ifdef NRF5_SPIM_USE_ANOM58_WAR
		/* Turn off PPI event. */
		NRF_PPI->CHENCLR = 1 << NRF5_ANOM58_PPI;
#endif

		/*
		 * Check if there's more data left to send. The NRF52
		 * SPI controller can only DMA up to 255 bytes in a
		 * single transfer, so larger transfers have to be
		 * split into chunks. Once we get to the point where
		 * we have less than a single chunk left, we can update
		 * the MAXCNT registers to pick the residual data,
		 * and then we can wake up the caller.
		 */

		if (spip->txcnt != 0 || spip->rxcnt != 0) {
			if (spip->txcnt < NRF5_SPIM_DMA_CHUNK)
				port->TXD.MAXCNT = spip->txcnt;
			if (spip->rxcnt < NRF5_SPIM_DMA_CHUNK)
				port->RXD.MAXCNT = spip->rxcnt;
			spip->port->TASKS_START = 1;
		} else
			_spi_isr_code(spip);
	 }

	return;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if NRF5_SPI_USE_SPI0 || defined(__DOXYGEN__)
/**
 * @brief   SPI0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector4C)
{
	CH_IRQ_PROLOGUE();
	serve_interrupt(&SPID1);
	CH_IRQ_EPILOGUE();
	return;
}
#endif

#if NRF5_SPI_USE_SPI1 || defined(__DOXYGEN__)
/**
 * @brief   SPI1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector50)
{
	CH_IRQ_PROLOGUE();
	serve_interrupt(&SPID2);
	CH_IRQ_EPILOGUE();
	return;
}
#endif

#if NRF5_SPI_USE_SPI2 || defined(__DOXYGEN__)
/**
 * @brief   SPI2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorCC)
{
	CH_IRQ_PROLOGUE();
	serve_interrupt(&SPID3);
	CH_IRQ_EPILOGUE();
	return;
}
#endif

#if NRF5_SPI_USE_SPI3 || defined(__DOXYGEN__)
/**
 * @brief   SPI3 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorFC)
{
	CH_IRQ_PROLOGUE();
	serve_interrupt(&SPID4);
	CH_IRQ_EPILOGUE();
	return;
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SPI driver initialization.
 *
 * @notapi
 */
void spi_lld_init(void)
{
#if NRF5_SPI_USE_SPI0
	spiObjectInit(&SPID1);
	SPID1.port = NRF_SPIM0;
#endif
#if NRF5_SPI_USE_SPI1
	spiObjectInit(&SPID2);
	SPID2.port = NRF_SPIM1;
#endif
#if NRF5_SPI_USE_SPI2
	spiObjectInit(&SPID3);
	SPID3.port = NRF_SPIM2;
#endif
#if NRF5_SPI_USE_SPI3
	spiObjectInit(&SPID4);
	SPID4.port = NRF_SPIM3;
#endif
}

/**
 * @brief   Configures and activates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_start(SPIDriver *spip)
{
	uint32_t config;

	if (spip->state == SPI_STOP) {
#if NRF5_SPI_USE_SPI0
	if (&SPID1 == spip)
		nvicEnableVector(SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn,
		    NRF5_SPI_SPI0_IRQ_PRIORITY);
#endif
#if NRF5_SPI_USE_SPI1
	if (&SPID2 == spip)
		nvicEnableVector(SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQn,
		    NRF5_SPI_SPI1_IRQ_PRIORITY);
#endif
#if NRF5_SPI_USE_SPI2
	if (&SPID3 == spip)
		nvicEnableVector(SPIM2_SPIS2_SPI2_IRQn,
		    NRF5_SPI_SPI2_IRQ_PRIORITY);
#endif
#if NRF5_SPI_USE_SPI3
	if (&SPID4 == spip)
		nvicEnableVector(SPIM3_IRQn,
		    NRF5_SPI_SPI2_IRQ_PRIORITY);
#endif
	}

	config = spip->config->lsbfirst ?
	    (SPI_CONFIG_ORDER_LsbFirst << SPI_CONFIG_ORDER_Pos) :
	    (SPI_CONFIG_ORDER_MsbFirst << SPI_CONFIG_ORDER_Pos);

	switch (spip->config->mode) {
		case 1:
			config |= (SPI_CONFIG_CPOL_ActiveLow
			    << SPI_CONFIG_CPOL_Pos);
			config |= (SPI_CONFIG_CPHA_Trailing
			    << SPI_CONFIG_CPHA_Pos);
			break;
		case 2:
			config |= (SPI_CONFIG_CPOL_ActiveHigh <<
			    SPI_CONFIG_CPOL_Pos);
			config |= (SPI_CONFIG_CPHA_Leading <<
			    SPI_CONFIG_CPHA_Pos);
			break;
		case 3:
			config |= (SPI_CONFIG_CPOL_ActiveHigh <<
			    SPI_CONFIG_CPOL_Pos);
			config |= (SPI_CONFIG_CPHA_Trailing <<
			    SPI_CONFIG_CPHA_Pos);
			break;
		default:
			config |= (SPI_CONFIG_CPOL_ActiveLow <<
			    SPI_CONFIG_CPOL_Pos);
			config |= (SPI_CONFIG_CPHA_Leading <<
			    SPI_CONFIG_CPHA_Pos);
			break;
	}

	/* Configuration.*/
	spip->port->CONFIG = config;
	spip->port->PSEL.SCK = spip->config->sckpad;
	spip->port->PSEL.MOSI = spip->config->mosipad;
	spip->port->PSEL.MISO = spip->config->misopad;
	spip->port->FREQUENCY = spip->config->freq;

	spip->port->ORC = 0xFF;
	spip->port->RXD.LIST = 1;
	spip->port->TXD.LIST = 1;

	spip->port->ENABLE =
	    (SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos);

  	spip->port->EVENTS_STARTED = 0;
  	(void)spip->port->EVENTS_STARTED;

	spip->port->EVENTS_END = 0;  
	(void)spip->port->EVENTS_END;

	/* Enable interrupts */
	spip->port->INTENSET = SPIM_INTENSET_END_Msk;
	(void)spip->port->INTENSET;

#ifdef NRF5_SPIM_USE_ANOM58_WAR

	/*
	 * Workaround for NRF52832 anomaly 58. The SPIM controller
	 * always clocks out enough bits for two bytes when asked 
	 * transfer only one bytes. This can cause problems with some
	 * slave devices, notably SD cards.
	 *
	 * To work around this, we set up a PPI event to
	 * stop the channel after the SPI clock pin changes
	 * state. The controller can't be stopped mid-byte, so
	 * the first byte will always complete and then the spurious
	 * second byte will be suppressed. This requires the use of
	 * one PPI channel and one GPIOTE channel, but we have
	 * some to spare.
	 */

	NRF_GPIOTE->CONFIG[NRF5_ANOM58_GPIOTE] =
	    GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos |
	    spip->port->PSEL.SCK << GPIOTE_CONFIG_PSEL_Pos |
	    GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos;

	NRF_PPI->CH[NRF5_ANOM58_PPI].EEP =
	    (uint32_t)&NRF_GPIOTE->EVENTS_IN[NRF5_ANOM58_GPIOTE];
	NRF_PPI->CH[NRF5_ANOM58_PPI].TEP =
	    (uint32_t)&spip->port->TASKS_STOP;
#endif

#ifdef notdef
	/* Work around for anomaly 109. Not sure if this is needed yet. */
	*(volatile uint32_t *)0x4006EC00 = 0x00009375;
	*(volatile uint32_t *)0x4006ED08 = 0x00000003;
	*(volatile uint32_t *)0x4006EC00 = 0x00009375;
#endif

	return;
}

/**
 * @brief   Deactivates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_stop(SPIDriver *spip)
{
	if (spip->state != SPI_STOP) {
		spip->port->TASKS_STOP = 1;
		spip->port->ENABLE  = SPIM_ENABLE_ENABLE_Msk;
		spip->port->INTENCLR = 0xFFFFFFFF; 
#if NRF5_SPI_USE_SPI0
		if (&SPID1 == spip)
			nvicDisableVector (SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn);
#endif
#if NRF5_SPI_USE_SPI1
		if (&SPID2 == spip)
			nvicDisableVector (SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQn);
#endif
#if NRF5_SPI_USE_SPI1
		if (&SPID3 == spip)
			nvicDisableVector (SPIM2_SPIS2_SPI2_IRQn);
#endif
#if NRF5_SPI_USE_SPI1
		if (&SPID4 == spip)
			nvicDisableVector (SPIM3_IRQn);
#endif
	}
	return;
}

/**
 * @brief   Asserts the slave select signal and prepares for transfers.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_select(SPIDriver *spip)
{
	palClearPad(IOPORT1, spip->config->sspad);
	return;
}

/**
 * @brief   Deasserts the slave select signal.
 * @details The previously selected peripheral is unselected.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_unselect(SPIDriver *spip)
{
	palSetPad(IOPORT1, spip->config->sspad);
	return;
}

/**
 * @brief   Ignores data on the SPI bus.
 * @details This function transmits a series of idle words on the SPI bus and
 *          ignores the received data. This function can be invoked even
 *          when a slave select signal has not been yet asserted.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be ignored
 *
 * @notapi
 */
void spi_lld_ignore(SPIDriver *spip, size_t n)
{
	spip->rxptr = NULL;
	spip->txptr = &spip->config->dummy;;
	spip->rxcnt = 0;
	spip->txcnt = n;
	port_fifo_preload(spip);
#ifdef NRF5_SPIM_USE_ANOM58_WAR
	if (n == 1)
		NRF_PPI->CHENSET = 1 << NRF5_ANOM58_PPI;
#endif
	spip->port->TASKS_START = 1;
	return;
}

/**
 * @brief   Exchanges data on the SPI bus.
 * @details This asynchronous function starts a simultaneous transmit/receive
 *          operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be exchanged
 * @param[in] txbuf     the pointer to the transmit buffer
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void spi_lld_exchange(SPIDriver *spip, size_t n,
                      const void *txbuf, void *rxbuf)
{
	spip->rxptr = rxbuf;
	spip->txptr = txbuf;
	spip->rxcnt = spip->txcnt = n;
	port_fifo_preload(spip);
#ifdef NRF5_SPIM_USE_ANOM58_WAR
	if (n == 1)
		NRF_PPI->CHENSET = 1 << NRF5_ANOM58_PPI;
#endif
	spip->port->TASKS_START = 1;
	return;
}

/**
 * @brief   Sends data over the SPI bus.
 * @details This asynchronous function starts a transmit operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to send
 * @param[in] txbuf     the pointer to the transmit buffer
 *
 * @notapi
 */

void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf)
{
	spip->rxptr = NULL;
	spip->txptr = txbuf;
	spip->rxcnt = 0;
	spip->txcnt = n;
	port_fifo_preload(spip);
#ifdef NRF5_SPIM_USE_ANOM58_WAR
	if (n == 1)
		NRF_PPI->CHENSET = 1 << NRF5_ANOM58_PPI;
#endif
	spip->port->TASKS_START = 1;
	return;
}

/**
 * @brief   Receives data from the SPI bus.
 * @details This asynchronous function starts a receive operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to receive
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf)
{
	spip->rxptr = rxbuf;
	spip->txptr = NULL;
	spip->rxcnt = n;
	spip->txcnt = 0;
	port_fifo_preload(spip);
#ifdef NRF5_SPIM_USE_ANOM58_WAR
	if (n == 1)
		NRF_PPI->CHENSET = 1 << NRF5_ANOM58_PPI;
#endif
	spip->port->TASKS_START = 1;
	return;
}

/**
 * @brief   Exchanges one frame using a polled wait.
 * @details This synchronous function exchanges one frame using a polled
 *          synchronization method. This function is useful when exchanging
 *          small amount of data on high speed channels, usually in this
 *          situation is much more efficient just wait for completion using
 *          polling than suspending the thread waiting for an interrupt.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] frame     the data frame to send over the SPI bus
 * @return              The received data frame from the SPI bus.
 */
uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame)
{
	spip->rxptr = &frame;
	spip->txptr = &frame;
	spip->rxcnt = 1;
	port_fifo_preload(spip);
#ifdef NRF5_SPIM_USE_ANOM58_WAR
	NRF_PPI->CHENSET = 1 << NRF5_ANOM58_PPI;
#endif
	spip->port->INTENCLR = SPIM_INTENSET_END_Msk;
	(void)spip->port->INTENCLR;
	spip->port->TASKS_START = 1;
	while (spip->port->EVENTS_END == 0)
		;
	spip->port->EVENTS_END = 0;
	(void)spip->port->EVENTS_END;
#ifdef NRF5_SPIM_USE_ANOM58_WAR
	NRF_PPI->CHENCLR = 1 << NRF5_ANOM58_PPI;
#endif
	spip->port->INTENSET = SPIM_INTENSET_END_Msk;
	(void)spip->port->INTENSET;
	return (frame);
}

#endif /* NRF5_USE_SPI_DMA */

#endif /* HAL_USE_SPI */

/** @} */
