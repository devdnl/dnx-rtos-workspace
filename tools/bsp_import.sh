#!/usr/bin/env bash

cd $(dirname $0)

if [ "$#" == "0" ]; then
    echo "Usage: $1 <bsp-file-path>"
    exit
fi

./configtool/configtool import="../../$1":"../../"
