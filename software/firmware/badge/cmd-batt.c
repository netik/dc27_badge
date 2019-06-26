#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"

#include "xpt2046_lld.h"
#include "xpt2046_reg.h"
#include "badge.h"

static void
cmd_batt (BaseSequentialStream *chp, int argc, char *argv[])
{
  uint16_t battv;
  (void)argv;
  if (argc > 0) {
    printf ("Usage: batt\n");
    return;
  }

  /*
   * From the XPT2046 datasheet:
   *
   * The battery voltage can vary from 0V to 6V, while maintaining the
   * voltage to the XPT2046 at 2.7V, 3.3V, etc. The input voltage
   * (VBAT) is The battery voltage can vary from divided down by 4 so
   * that a 5.5V battery voltage is represented as 1.375V to the
   * ADC. This simplifies the multiplexer and control logic. In order
   * to minimize the power consumption, the divider is only on during
   * the sampling period when A2 = 0, A1 = 1, and A0 = 0 (see Table 1
   * for the relationship between the control bits and configuration
   * of the XPT2046).
   *
   * jna: this appears to be the ADC multiplexer inputs. bits 6-4 (A2-A0)
   *      of the control bits in the control byte. 
   *
   * The ADC range is 0.125 to 1.375V (assuming this is 0->255 ? )
   *
   * VBAT is on QFN pin 11, pin 7 on the TSSOP, or pin G6 on the VFBGA
   * 
   * The xptGET call truncates this to 12 bits.

   */

  battv = xptGet(XPT_CHAN_VBAT | XPT_CTL_SDREF | );
  printf ("battery voltage = %d\n", battv);
  return;
}

orchard_command("batt", cmd_batt);
