#!/bin/bash

sudo apt-get update
sudo apt-get install -y git g++ cmake libsqlite3-dev libi2c-dev libusb-1.0-0-dev netcat
sudo apt-get install -y wiringpi

git clone --depth=1 https://github.com/myriadrf/LimeSuite
cd LimeSuite
mkdir dirbuild
cd dirbuild
cmake ../
make
sudo make install
sudo ldconfig
cd ../udev-rules/
chmod +x install.sh
sudo ./install.sh
cd ../../

git clone https://github.com/f4dvk/limesdr_toolbox
cd limesdr_toolbox

sudo apt-get install -y libfftw3-dev
git clone https://github.com/F5OEO/libdvbmod
cd libdvbmod/libdvbmod
make
cd ../DvbTsToIQ/
make
cp dvb2iq ../../../../bin/
cd ../../

make
make dvb

echo "alias transpondeur='/home/pi/limesdr_toolbox/menu.sh'" >> /home/pi/.bash_aliases

sudo reboot
