#!/bin/bash

#PTT=GPIO 21 / Header 40
PTT=21

transmit=0;

gpio -g mode $PTT in
gpio -g mode $PTT up

##################### TRANSMIT ##############

do_transmit()
{
        home/pi/limesdr_toolbox/transpondeur.sh >/dev/null 2>/dev/null &
}

do_stop_transmit()
{
        sudo killall limesdr_forward 2>/dev/null
        sleep 1
        home/pi/limesdr_toolbox/limesdr_stopchannel >/dev/null 2>/dev/null
}

do_process_button()
{

        if [ `gpio -g read $PTT` = 0 ]&&[ "$transmit" = 0 ] ; then

                transmit=1;
                do_transmit
                echo "Emission"
        fi

        if [ `gpio -g read $PTT` = 1 ]&&[ "$transmit" = 1 ] ; then

                do_stop_transmit
                transmit=0;
                echo "Standby"
	fi
}

##################### MAIN PROGRAM ##############

while true; do

        do_process_button

sleep 0.5

done
