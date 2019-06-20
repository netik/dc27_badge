#!/bin/bash

# two JTAGS, one (human) programmer.
#
# if you want this script to work, add your olimex serial numbers below.
#
# edit each one and add the device serial number
# you can get this on mac via system preferences > system report
#
# add:
# ftdi_serial ...serial...
# to each file.
#
# this will open up ocd instances to both olimexes, if you have two installed.
SERIALS="OLAC6A5A OLHLB2K"
serialarr=($SERIALS)

NUM=1

if [ "$1" == "" ];
then
    # show a menu, nothing was requested

    for i in $SERIALS;
    do
        echo "$NUM) $i"
        NUM=$(( $NUM + 1))
    done
    
    echo -n "Which? [1]: "
    read X
else
    X=$1
fi

if [ "$X" == "" ]; then
    X=1
fi

SERIAL=${serialarr[$(( $X - 1 ))]}

BASE_GDB=$(( 3333 + $X - 1 ))
BASE_TELNET=$(( 4444 + $X - 1 ))
BASE_TCL=$(( 6666 + $X - 1 ))

echo
echo "Ports: $BASE_GDB $BASE_TELNET $BASE_TCL"
echo "Serial: $SERIAL"

openocd -f interface/ftdi/olimex-arm-usb-ocd-h.cfg	\
	-f interface/ftdi/olimex-arm-jtag-swd.cfg	\
	-f target/nrf52.cfg				\
        -c "gdb_port ${BASE_GDB}" \
        -c "telnet_port ${BASE_TELNET}" \
        -c "tcl_port ${BASE_TCL}" \
        -c "ftdi_serial ${SERIAL}" \
	-c "gdb_flash_program enable"			\
	-c "gdb_breakpoint_override hard"		\
	-c "nrf52.cpu configure -rtos ChibiOS" 
