#!/usr/bin/env bash

cd $(dirname $0)

if [ "$1" == "" ]; then
    ./configtool/configtool
else
    ./configtool/configtool $1 PLUGINS-DIR=../configtool-plugins/
fi
