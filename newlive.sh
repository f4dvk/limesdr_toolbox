mkfifo audioin.wav
arecord -f S16_LE -r 24000 -c 2 -B 40000 -D plughw:1,0 > audioin.wav &
mkfifo iq
SYMBOLRATE=1000
FECNUM=5
FECDEN=6
BITRATE_TS=$(./limesdr_dvb -s "$SYMBOLRATE"000 -f $FECNUM"/"$FECDEN -m DVBS2 -c QPSK  -d)
let VIDEOBITRATE=BITRATE_TS-120000
let BITRATE_TS=BITRATE_TS

 ../avc2ts/avc2ts -a audioin.wav -z 12000 -t 0 -e /dev/video1 -m $BITRATE_TS -b $VIDEOBITRATE -x 704 -y 576  -f 25 -d 0 -o /dev/stdout -n 230.0.0.10:10000  |sudo ./limesdr_dvb  -s "$SYMBOLRATE"000 -f $FECNUM/$FECDEN -r 2 -m DVBS2 -c QPSK  -t 744.5e6 -g 0.7 -q 0


