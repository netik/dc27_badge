#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"

#include "nrf_soc.h"
#include "nrf_sdm.h"
#include "badge.h"

void randInit(void) {
  // might not need this.
  return;
}

void
srandom (unsigned int seed)
{
  (void)seed;
}

long
random (void)
{
  uint8_t sdenabled = 0;
  long random_buffer;

  rngAcquireUnit (&RNGD1);

  sd_softdevice_is_enabled (&sdenabled);
 
 if (sdenabled == TRUE)
    sd_rand_application_vector_get ((uint8_t *)&random_buffer, 4);
  else {
    rngWrite (&RNGD1, (uint8_t *)&random_buffer, 4, OSAL_MS2I(100));
  }

  rngReleaseUnit (&RNGD1);

  return random_buffer;
}

uint8_t randByte(void) {
  uint8_t sdenabled = 0;
  uint8_t random_buffer;

  rngAcquireUnit (&RNGD1);

  sd_softdevice_is_enabled (&sdenabled);

  if (sdenabled == TRUE)
    sd_rand_application_vector_get (&random_buffer, 1);
  else {
    rngWrite (&RNGD1, &random_buffer, 1, OSAL_MS2I(100));
  }

  rngReleaseUnit (&RNGD1);

  return random_buffer;
}

// mostly used for entity ID's. 
uint16_t randUInt16(void) {
  uint8_t sdenabled = 0;
  uint16_t random_buffer;

  rngAcquireUnit (&RNGD1);

  sd_softdevice_is_enabled (&sdenabled);

  if (sdenabled == TRUE)
    sd_rand_application_vector_get ((uint8_t*)&random_buffer, 1);
  else {
    rngWrite (&RNGD1, (uint8_t *)&random_buffer, sizeof(uint16_t), OSAL_MS2I(100));
  }

  rngReleaseUnit (&RNGD1);

  return random_buffer;
}

uint8_t randRange(uint8_t min, uint8_t max) {
  if (max == min) {
    return min;
  }
  return (randByte() % (max - min)) + min;
}
