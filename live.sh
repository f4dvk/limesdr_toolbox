mkfifo audioin.wav
arecord -f S16_LE -r 48000 -c 2 -B 100000 -D plughw:1,0 > audioin.wav &
mkfifo iq
SYMBOLRATE=250
FECNUM=1
FECDEN=2
BITRATE_TS=$(../libdvbmod/DvbTsToIQ/dvb2iq -s $SYMBOLRATE -f $FECNUM"/"$FECDEN -m DVBS2 -c QPSK -v -d)
let VIDEOBITRATE=BITRATE_TS-100000
let BITRATE_TS=BITRATE_TS

 ../avc2ts/avc2ts -a audioin.wav -z 4000 -t 0 -e /dev/video1 -m $BITRATE_TS -b $VIDEOBITRATE -x 352 -y 288  -f 25 -d 400 -o /dev/stdout -n 230.0.0.10:10000  |../libdvbmod/DvbTsToIQ/dvb2iq  -s $SYMBOLRATE -f $FECNUM/$FECDEN -r 4 -m DVBS2 -c QPSK -v | sudo ./limesdr_send -f 744.50e6 -b 2.5e6 -s "$SYMBOLRATE"000 -g 0.65 -p 0.05 -a BAND2 -r 4 -l 10000 


