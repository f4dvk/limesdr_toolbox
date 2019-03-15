
SYMBOLRATE=250
FECNUM=5
FECDEN=6
BITRATE_TS=$(../libdvbmod/DvbTsToIQ/dvb2iq -s $SYMBOLRATE -f $FECNUM"/"$FECDEN -m DVBS2 -c QPSK -v -d)
let VIDEOBITRATE=BITRATE_TS-200000
let BITRATE_TS=BITRATE_TS

mnc -l -p 10000 230.0.0.10 |buffer |../libdvbmod/DvbTsToIQ/dvb2iq  -s $SYMBOLRATE -f $FECNUM/$FECDEN -r 2 -m DVBS2 -c QPSK -v | sudo ./limesdr_send -f 744.50e6 -b 2.5e6 -s "$SYMBOLRATE"000 -g 0.65 -p 0.05 -a BAND2 -r 2 -l 40000 


