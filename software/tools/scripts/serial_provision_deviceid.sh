#!/bin/sh

# This script is similar to the original provision script, except it
# obtains the device ID from the nRF52840 MCU via JTAG and uses that
# as the serial number. This requires OpenOCD to be running on the
# local host.

echo
echo "daBomb cp2102 provision Ready."

while [ /bin/true ];
do
    
    /bin/echo "Connect cables and hit ENTER or Q to quit."
    /bin/echo -n "==> "
    
    read X
    
    if [ "$X" == "q" -o "$X" == "Q" ]; then
        echo "[*] Bye!"
        exit
    fi


    echo "[*] start openocd"
    X=1
    BASE_GDB=$(( 3333 + $X - 1 ))
    BASE_TELNET=$(( 4444 + $X - 1 ))
    BASE_TCL=$(( 6666 + $X - 1 ))
    SERIAL="OLAC6A5A"

    openocd -f interface/ftdi/olimex-arm-usb-ocd-h.cfg      \
        -f interface/ftdi/olimex-arm-jtag-swd.cfg       \
        -f target/nrf52.cfg                             \
        -c "gdb_port ${BASE_GDB}" \
        -c "telnet_port ${BASE_TELNET}" \
        -c "tcl_port ${BASE_TCL}" \
        -c "ftdi_serial ${SERIAL}" \
        -c "gdb_flash_program enable"                   \
        -c "gdb_breakpoint_override hard"               \
        -c "nrf52.cpu configure -rtos ChibiOS"  &
    
    echo "[*] Attempting serial provision for 2102 ..."
    
    NEWSERIAL=`arm-none-eabi-gdb -q --batch --command=scripts/deviceid.txt | grep DEVICEID | cut -c10-27`
    ./bin/cp2102 -l src/cp2102n_default.txt
    ./bin/cp2102 -x 500 -p "da Bomb" -m "Team Ides" -g on -s ${NEWSERIAL}
    
    echo "[*] Provisioned serial ${NEWSERIAL}..."
    echo "[*] Serial number will not be returned by chip until full power cycle."
    echo
    echo `date` ${NEWSERIAL} >> serials.txt
    echo

    echo "[*] stop open ocd"
    pkill openocd

    echo "[*] provision."
    time ( cd /Users/jna/src/badge/dc27_badge/hardware/programming;  ./FINAL-provision-nrf52-olimex.sh)
done

