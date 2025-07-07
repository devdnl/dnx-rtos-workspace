#!/usr/bin/env bash

cd $(dirname $0)

if [ "$#" == "0" ]; then
    echo "Usage: $1 <bsp-file-path> [optimize-arch-name]"
    exit
fi

./configtool/configtool export="../../$1":"../../,app-romfs/config,config"

if [ "$#" == "2" ]; then
    lua ./optimize_bsp.lua "../$1" "$2"
fi
