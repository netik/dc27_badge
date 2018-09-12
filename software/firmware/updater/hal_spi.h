/*
 * This is here to override the calls to spiAcquireBus() and spiReleaseBus()
 * in the mmc_spi_lld.c code. In the updater program, we only use the
 * SPI channel for access to the SD card and there are no other threads
 * running that can race with us, so the mutual exclusion is not needed.
 * This saves us from having to link spi.o into the updater image.
 */

#define spiAcquireBus(x)
#define spiReleaseBus(x)
