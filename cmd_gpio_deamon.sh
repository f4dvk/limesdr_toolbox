#!/bin/bash

if [ "$1" == "1" ]; then
  if ! grep -q cmd_gpio.sh /etc/rc.local; then
    sudo sed -i "s/\"exit 0\"/\"exit\"/g" /etc/rc.local
    sudo sed -i "/exit 0/i\/home/pi/limesdr_toolbox/cmd_gpio.sh\n" /etc/rc.local
    sudo sed -i "/^$/d" /etc/rc.local
  fi
fi

if [ "$1" == "0" ]; then
  if grep -q "/home/pi/limesdr_toolbox/cmd_gpio.sh" /etc/rc.local; then
    sudo sed -i "/\/home\/pi\/limesdr_toolbox\/cmd_gpio.sh/d" /etc/rc.local
    sudo sed -i "/^$/d" /etc/rc.local
  fi
fi

exit
