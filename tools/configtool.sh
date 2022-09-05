#!/usr/bin/env bash

cd $(dirname $0)

./configtool/configtool $1 PLUGINS-DIR=../configtool-plugins/
