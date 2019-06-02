#!/bin/bash

echo
echo "daBomb cp2102 provision Ready."
echo -n "Connect cable and hit ENTER! -->"

read X

echo "[*] Attempting serial provision for 2102 ..."

NEWSERIAL=`uuidgen | sed 's/-//g' | cut -c1-16`
./bin/cp2102 -p "da Bomb" -m "Team Ides" -g on -s ${NEWSERIAL}

echo "[*] Provisioned serial ${NEWSERIAL}..."
echo `date` ${NEWSERIAL} >> serials.txt

