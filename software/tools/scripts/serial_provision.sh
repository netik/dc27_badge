#!/bin/sh

echo
echo "daBomb cp2102 provision Ready."

while [ /bin/true ];
do
    
    /bin/echo "Connect cable and hit ENTER or Q to quit."
    /bin/echo -n "==> "
    
    read X
    
    if [ "$X" == "q" -o "$X" == "Q" ]; then
        echo "[*] Bye!"
        exit
    fi
    
    echo "[*] Attempting serial provision for 2102 ..."
    
    NEWSERIAL=`uuidgen | sed 's/-//g' | cut -c1-16`
    ./bin/cp2102 -p "da Bomb" -m "Team Ides" -g on -s ${NEWSERIAL}
    

    echo "[*] Provisioned serial ${NEWSERIAL}..."
    echo "[*] Serial number will not be returned by chip until full power cycle."
    echo
    echo `date` ${NEWSERIAL} >> serials.txt
    echo
done

