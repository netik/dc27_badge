/*
    RNG for ChibiOS - Copyright (C) 2016 Stephane D'Alu

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    NRF5/LLD/hal_rng_lld.c
 * @brief   NRF5 RNG subsystem low level driver source.
 *
 * @addtogroup RNG
 * @{
 */

#include "hal.h"

#if (HAL_USE_RNG == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @brief   RNG default configuration.
 */
static const RNGConfig default_config = {
  .digital_error_correction = 1,
};

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief RNGD1 driver identifier.*/
#if NRF5_RNG_USE_RNG0 || defined(__DOXYGEN__)
RNGDriver RNGD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   RNG interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(Vector74)
{
  NRF_RNG_Type *rng;

  rng = RNGD1.rng;

  OSAL_IRQ_PROLOGUE();
  osalSysLock ();

  rng->EVENTS_VALRDY = 0;
#if CORTEX_MODEL >= 4
  (void)rng->EVENTS_VALRDY;
#endif

  osalThreadResumeI (&RNGD1.tr, MSG_OK);
  osalSysUnlock ();
  OSAL_IRQ_EPILOGUE();
  return;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level RNG driver initialization.
 *
 * @notapi
 */
void rng_lld_init(void) {
  rngObjectInit(&RNGD1);
  RNGD1.rng    = NRF_RNG;
  RNGD1.irq    = RNG_IRQn;
}

/**
 * @brief   Configures and activates the RNG peripheral.
 *
 * @param[in] rngp      pointer to the @p RNGDriver object
 *
 * @notapi
 */
void rng_lld_start(RNGDriver *rngp) {
  NRF_RNG_Type *rng = rngp->rng;

  /* If not specified, set default configuration */
  if (rngp->config == NULL)
    rngp->config = &default_config;

  /* Configure digital error correction */
  if (rngp->config->digital_error_correction) 
    rng->CONFIG |=  RNG_CONFIG_DERCEN_Msk;
  else
    rng->CONFIG &= ~RNG_CONFIG_DERCEN_Msk;

  /* Enable vector */
  nvicEnableVector (RNG_IRQn, NRF5_RNG_RNG0_IRQ_PRIORITY);

  /* Set interrupt mask */
  rng->INTENSET      = RNG_INTENSET_VALRDY_Msk;

}


/**
 * @brief   Deactivates the RNG peripheral.
 *
 * @param[in] rngp      pointer to the @p RNGDriver object
 *
 * @notapi
 */
void rng_lld_stop(RNGDriver *rngp) {
  NRF_RNG_Type *rng = rngp->rng;

  /* Clear interrupt mask */
  rng->INTENCLR      = RNG_INTENSET_VALRDY_Msk;

  /* Disable vector */
  nvicDisableVector (RNG_IRQn);
}


/**
 * @brief   Write random bytes;
 *
 * @param[in] rngp      pointer to the @p RNGDriver object
 * @param[in] n         size of buf in bytes
 * @param[in] buf       @p buffer location
 *
 * @notapi
 */
msg_t rng_lld_write(RNGDriver *rngp, uint8_t *buf, size_t n,
                    systime_t timeout) {
  NRF_RNG_Type *rng = rngp->rng;
  msg_t msg;
  size_t i;

  if (rngp->state != RNG_READY)
      return (MSG_TIMEOUT);

  /* Start */
  rng->TASKS_START = 1;

  for (i = 0; i < n; i++) {
    msg = osalThreadSuspendTimeoutS (&rngp->tr, timeout);
    if (msg != MSG_OK)
      break;

    /* Read byte */
    buf[i] = (char)rng->VALUE;

  }

  /* Stop */
  rng->TASKS_STOP = 1;

  /* Mark as read */
  rng->EVENTS_VALRDY = 0;
#if CORTEX_MODEL >= 4
  (void)rng->EVENTS_VALRDY;
#endif

  return (msg);
}

#endif /* HAL_USE_RNG */

/** @} */
