#!/usr/bin/env bash

cd $(dirname $0)

create_gdb_command() {
    echo "target remote localhost:3333" >> gdbcmd
    echo "monitor reset halt" >> gdbcmd
    echo "load" >> gdbcmd
    echo "monitor reset" >> gdbcmd
}

echo "Waiting for a connection to a target..."
create_gdb_command
arm-none-eabi-gdb --quiet --batch -x gdbcmd ../build/project.elf
rm -f gdbcmd

date "+Done [%H:%M:%S]"
