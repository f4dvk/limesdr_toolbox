mkfifo audioin.wav
arecord -f S16_LE -r 24000 -c 2 -B 40000 -D plughw:1,0 > audioin.wav &
mkfifo iq
FREQ=2403
SYMBOLRATE=1000
FECNUM=5
FECDEN=6
MODE=DVBS2
GAIN=0.8

BITRATE_TS=$(./limesdr_dvb -s "$SYMBOLRATE"000 -f $FECNUM"/"$FECDEN -m $MODE -c QPSK  -d)
let VIDEOBITRATE=BITRATE_TS-120000
let BITRATE_TS=BITRATE_TS

 ../avc2ts/avc2ts -a audioin.wav -z 12000 -t 0 -e /dev/video1 -m $BITRATE_TS -b $VIDEOBITRATE -x 704 -y 576  -f 25 -d 0 -o /dev/stdout -n 230.0.0.10:10000  |sudo ./limesdr_dvb  -s "$SYMBOLRATE"000 -f $FECNUM/$FECDEN -r 2 -m $MODE -c QPSK  -t "$FREQ"e6 -g $GAIN -q 0


