#!/usr/bin/env bash

if [ "$1" == "" ]; then
    dev=/dev/ttyUSB0
else
    dev=$1
fi

read -p '(m)inicom or (s)creen: ' terminal

if [ "$terminal" == "m" ]; then minicom -C /tmp/minicom-$(basename "$dev").log -D "$dev" -b 460800; fi
if [ "$terminal" == "s" ]; then screen -L -Logfile /tmp/screen-$(basename "$dev").log "$dev" 460800; fi

