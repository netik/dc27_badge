#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"

#include "nrf_soc.h"
#include "badge.h"

void randInit(void) {
  // might not need this.
  return;
}

uint8_t randByte(void) {
  static uint8_t random_buffer;
  sd_rand_application_vector_get (&random_buffer, 1);

  return random_buffer;
}

uint8_t randRange(uint8_t min, uint8_t max) {
  if (max == min) {
    return min;
  }
  return (randByte() % (max - min)) + min;
}
