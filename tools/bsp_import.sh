#!/usr/bin/env bash

cd $(dirname $0)

if [ "$#" == "0" ]; then
    echo "Usage: $1 <bsp-file-path>"
    exit
fi

sh ../kernel/tools/configtool.sh import="../$1":"../"
