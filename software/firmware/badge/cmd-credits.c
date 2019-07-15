#include "ch.h"
#include "hal.h"
#include "shell.h"

#include <stdio.h>
#include <stdlib.h>

#include "nrf_soc.h"
#include "nrf52temp_lld.h"

#include "badge.h"

static const char *credits_txt[] = {
  "",
  "+-----------------------------------------------------------------------------+",
  "| da Bomb! DEF CON 27 (2019)                    | Special thanks to our       |",
  "|                                               | Kickstarter Petty Officers  |",
  "| Credits                                       |                             |",
  "|                                               |  @Growhio1                  |",
  "| John Adams (@netik) - PCB/HW/SW               |  @rj_chap                   |",
  "| Bill Paul - HW/SW/Firmware/Board bring-up     |  C                          |",
  "| Egan Hirvela - Game Mechanics                 |  Ciph3r                     |",
  "| Devon Dossett - Sprite Library                |  DrydenMaker                |",
  "| Seth Little - Challenge Coin Design           |  Elestereo                  |",
  "|                                               |  gdb                        |",
  "|                                               |  James Houston              |",
  "| Powered by ChibiOS                            |  JWShipp                    |",
  "| http://www.chibios.org                        |  Martin Lebel               |",
  "|                                               |  mikea                      |",
  "|                                               |  nn_amon                    |",
  "| Twitter: @spqrbadge                           |  Scott \"Phayl\" Perzan       |",
  "|                                               |  The Hat                    |",
  "|                                               |  XThree13                   |",
  "| Source Code, PCB, and Schematics:             |  Shad Epolito               |",
  "| https://github.com/netik/dc27_badge           |                             |",
  "|                                               | In memoriam of Nolan Berry  |",
  "| For more information, see https://ides.team   |                aka d3vnull  |",
  "+-----------------------------------------------+-----------------------------+",
  NULL
};

static void
cmd_credits(BaseSequentialStream *chp, int argc, char *argv[])
{
  uint8_t i = 0;;

  while (credits_txt[i] != NULL) {
    printf("%s\n", credits_txt[i]);
    i++;
  }

  printf("\n");

  return;
}

orchard_command("credits", cmd_credits);
