#!/usr/bin/env bash

if [ "$1" == "" ]; then
    dev=/dev/ttyACM0
else
    dev=$1
fi

#minicom -C /tmp/sterownik_domu.log -D "$dev" -b 460800
screen -L -Logfile /tmp/screen-hmi.log "$dev" 460800
