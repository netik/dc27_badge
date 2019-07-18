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
  "+-----------------------------------------------+-----------------------------+",
  "| da Bomb! DEF CON 27 (2019) -- Credits         | Special thanks to our       |",
  "|                                               | Kickstarter Petty Officers  |",
  "| Team IDES ----------------- https://ides.team |  -~-.-~-.-~.-~-.-~-.-~.-~-  |",
  "|                                               |                             |",
  "| John Adams (@netik) - PCB/HW/SW               |  @Growhio1    @rj_chap      |",
  "| Bill Paul - HW/SW/Firmware/Board bring-up     |  C            Ciph3r        |",
  "| Egan Hirvela - Game Mechanics                 |  DrydenMaker  Elestere      |",
  "| Devon Dossett - Sprite Library                |  gdb          James Houston |",
  "| Seth Little - Challenge Coin Design           |  JWShipp      Martin Lebel  |",
  "|                                               |  mikea        nn_amon       |",
  "|             ~^~^~'====>`~^~^~                 |  Scott \"Phayl\" Perzan       |",
  "|                                               |  The Hat                    |",
  "| Powered by ChibiOS                            |  XThree13                   |",
  "| http://www.chibios.org                        |  Shad Epolito               |", 
  "|                                               |                             |", 
  "| Twitter: @spqrbadge                           |                             |", 
  "|                                               |                             |", 
  "| Source Code, PCB, and Schematics              |                             |", 
  "|          https://github.com/netik/dc27_badge  |    in memoriam Nolan Berry  |",
  "|                                               |                aka d3vnull  |",
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
