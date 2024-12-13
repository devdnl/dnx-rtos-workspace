#!/usr/bin/env bash

cd $(dirname $0)

create_load_command() {
    echo "target remote localhost:3333" > gdbcmd
    echo "monitor reset" >> gdbcmd
    echo "load" >> gdbcmd
}

create_reset_command() {
    echo "target remote localhost:3333" > gdbcmd
    echo "monitor reset" >> gdbcmd
}

echo "Waiting for a connection to a target..."

create_load_command
arm-none-eabi-gdb --quiet --batch -x gdbcmd $1

sleep 1
create_reset_command
arm-none-eabi-gdb --quiet --batch -x gdbcmd $1

rm -f gdbcmd

date "+Done [%H:%M:%S]"
