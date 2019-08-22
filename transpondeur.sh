#!/bin/bash

PCONFIGFORWARD="/home/pi/limesdr_toolbox/forward_config.txt"

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

FREQ_OUTPUT=$(get_config_var freqoutput $PCONFIGFORWARD)
FREQ_INPUT=$(get_config_var freqinput $PCONFIGFORWARD)
RX_GAIN=$(get_config_var rxgain $PCONFIGFORWARD)
TX_GAIN=$(get_config_var txgain $PCONFIGFORWARD)
BW_CAL=$(get_config_var bwcal $PCONFIGFORWARD)
SAMPLERATE=$(get_config_var samplerate $PCONFIGFORWARD)

sudo /home/pi/limesdr_toolbox/limesdr_forward -b $BW_CAL -s $SAMPLERATE -g $RX_GAIN -f $FREQ_INPUT"e6" -G $TX_GAIN -F $FREQ_OUTPUT"e6" >/dev/null 2>/dev/null

exit
