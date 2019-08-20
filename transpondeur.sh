#!/bin/bash

PCONFIGFORWARD="$PWD/forward_config.txt"

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

$PWD/limesdr_forward -b $BW_CAL -s $SAMPLERATE -f $FREQ_INPUT"e6" -F $FREQ_OUTPUT"e6" >/dev/null 2>/dev/null

exit
