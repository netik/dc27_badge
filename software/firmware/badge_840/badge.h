#ifndef _BADGE_H_
#define _BADGE_H_
#include "chprintf.h"

#define printf(fmt, ...)                                        \
    chprintf((BaseSequentialStream*)&SD1, fmt, ##__VA_ARGS__)

/* Orchard command linker set handling */

#define orchard_command_start()						\
	static char start[4] __attribute__((unused,			\
	    aligned(4), section(".chibi_list_cmd_1")))

#define orchard_commands()      (const ShellCommand *)&start[4]

#define orchard_command(_name, _func)					\
	const ShellCommand _orchard_cmd_list_##_func			\
	__attribute__((used, aligned(4),				\
	     section(".chibi_list_cmd_2_" _name))) = { _name, _func }

#define orchard_command_end()						\
	const ShellCommand _orchard_cmd_list_##_func			\
	__attribute__((used, aligned(4),				\
	    section(".chibi_list_cmd_3_end"))) = { NULL, NULL }

/*
 * The fault info address is chosen to be inside the reserved
 * SoftDevice RAM region. This is so that we know it won't be
 * overwritten during ChibiOS bootstrap.
 */

#define NRF_FAULT_INFO_MAGIC	0xFEEDFACE
#define NRF_FAULT_INFO_ADDR	0x20002000

#endif /* _BADGE_H_ */
