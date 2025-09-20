#!/usr/bin/env bash

cd $(dirname $0)

echo "Creating custom linker script..."
(./configtool/configtool tolua="$1") | (lua  ./create_linker_script.lua) > "$2"
