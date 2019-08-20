#!/bin/bash

git clone https://github.com/f4dvk/limesdr_toolbox
cd limesdr_toolbox
make

chmod +x transpondeur.sh
chmod +x menu.sh

exit
