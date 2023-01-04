#!/usr/bin/env bash

openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "init"
