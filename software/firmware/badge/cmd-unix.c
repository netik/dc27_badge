
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"

#include "chprintf.h"
#include "userconfig.h"
#include "shell.h"

#include "badge.h"

/*
 * cmd-unix.c
 * J. Adams
 * 1/19/2017
 *
 * a collection of commands that have a unix look and feel but are
 * not...
 */

void cmd_whoami(BaseSequentialStream *chp, int argc, char *argv[])
{
  userconfig *config = getConfig();
  (void)argc;
  (void)argv;

  chprintf(chp, "%s\r\n", config->name);

}

orchard_command("whoami", cmd_whoami);

void cmd_uname(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  chprintf(chp, "ChibiOS %s %s; %s %s %s\r\n",
           CH_KERNEL_VERSION,
           BOARD_NAME,
           PORT_ARCHITECTURE_NAME,
           __DATE__,
           __TIME__);

}

orchard_command("uname", cmd_uname);
