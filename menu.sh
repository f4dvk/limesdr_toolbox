#!

PCONFIGFORWARD="$PWD/forward_config.txt"
status="0"

get_config_var() {
lua - "$1" "$2" <<EOF
local key=assert(arg[1])
local fn=assert(arg[2])
local file=assert(io.open(fn))
for line in file:lines() do
local val = line:match("^#?%s*"..key.."=(.*)$")
if (val ~= nil) then
print(val)
break
end
end
EOF
}

set_config_var() {
lua - "$1" "$2" "$3"<<EOF > "$3.bak2"
local key=assert(arg[1])
local value=assert(arg[2])
local fn=assert(arg[3])
local file=assert(io.open(fn))
local made_change=false
for line in file:lines() do
if line:match("^#?%s*"..key.."=.*$") then
line=key.."="..value
made_change=true
end
print(line)
end
if not made_change then
print(key.."="..value)
end
EOF
mv "$3.bak2" "$3"
}

FREQ_OUTPUT=$(get_config_var freqoutput $PCONFIGFORWARD)
FREQ_INPUT=$(get_config_var freqinput $PCONFIGFORWARD)
RX_GAIN=$(get_config_var rxgain $PCONFIGFORWARD)
TX_GAIN=$(get_config_var txgain $PCONFIGFORWARD)
BW_CAL=$(get_config_var bwcal $PCONFIGFORWARD)
SAMPLERATE=$(get_config_var samplerate $PCONFIGFORWARD)

if [ "$RX_GAIN" = "-1" ]; then
    RX_GAIN2=""
else
    RX_GAIN2=$RX_GAIN
fi

do_forward_setup()
{

if FREQ_I=$(whiptail --inputbox "Choisir la fréquence d'entrée (en MHZ) " 8 78 $FREQ_INPUT --title "Fréquence d'entrée transpondeur" 3>&1 1>&2 2>&3); then
    FREQ_INPUT=$FREQ_I
    set_config_var freqinput "$FREQ_INPUT" $PCONFIGFORWARD
fi

if FREQ_O=$(whiptail --inputbox "Choisir la fréquence de sortie (en MHZ) " 8 78 $FREQ_OUTPUT --title "Fréquence de sortie transpondeur" 3>&1 1>&2 2>&3); then
    FREQ_OUTPUT=$FREQ_O
    set_config_var freqoutput "$FREQ_OUTPUT" $PCONFIGFORWARD
fi

if GAIN_R=$(whiptail --inputbox "Choisir le gain RX (de 0.00 à 1; vide: non utilisé)" 8 78 $RX_GAIN2 --title "Gain RX" 3>&1 1>&2 2>&3); then
    RX_GAIN=$GAIN_R
    RX_GAIN2=$RX_GAIN
    if [ "$RX_GAIN" = "" ]; then
        RX_GAIN="-1"
    fi
    set_config_var rxgain "$RX_GAIN" $PCONFIGFORWARD
fi

if GAIN_T=$(whiptail --inputbox "Choisir le gain TX (de 0.00 à 1)" 8 78 $TX_GAIN --title "Gain TX" 3>&1 1>&2 2>&3); then
    TX_GAIN=$GAIN_T
    set_config_var txgain "$TX_GAIN" $PCONFIGFORWARD
fi

if BW=$(whiptail --inputbox "Choisir la bande passante de calibration (mini 4e6 si gain RX < à 0.6, sinon 2.5e6 mini.)" 8 78 $BW_CAL --title "Bande Passante Calibration" 3>&1 1>&2 2>&3); then
    BW_CAL=$BW
    set_config_var bwcal "$BW_CAL" $PCONFIGFORWARD
fi

if SR=$(whiptail --inputbox "Choisir le samplerate (défaut 1.2e6)" 8 78 $SAMPLERATE --title "Samplerate" 3>&1 1>&2 2>&3); then
    SAMPLERATE=$SR
    set_config_var samplerate "$SAMPLERATE" $PCONFIGFORWARD
fi

}

do_stop()
{
  sudo killall limesdr_forward 2>/dev/null
  sleep 1
  $PWD/limesdr_stopchannel >/dev/null 2>/dev/null
}

do_status()
{
	whiptail --title "Transpondeur ""$FREQ_INPUT""MHZ ""$FREQ_OUTPUT""MHZ" --msgbox "Actif" 8 78
	do_stop
}

while [ "$status" -eq 0 ]
   do

menuchoice=$(whiptail --title "Transpondeur" --menu "Menu" 20 102 12 \
 "0 Transpondeur" " $FREQ_INPUT => ""$FREQ_OUTPUT"MHZ \
 "1 Config" " $FREQ_INPUT => ""$FREQ_OUTPUT MHZ, Gain RX: $RX_GAIN2, Gain TX: $TX_GAIN, BW: $BW_CAL, Samplerate: $SAMPLERATE" \
 3>&2 2>&1 1>&3)

       case "$menuchoice" in
   0\ *) "$PWD/transpondeur.sh" >/dev/null 2>/dev/null &
   do_status;;
   1\ *)
   do_forward_setup;;
   *)	 status=1
   whiptail --title "Au revoir" --msgbox "Merci d'avoir utilisé le transpondeur" 8 78
   ;;
       esac
   done
