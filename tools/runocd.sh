#!/usr/bin/env bash

get_target_name() {
    echo $(cat ./config/app-system/project/project_flags.h | grep  -oP '^#\s*define\s+__CPU_ARCH__\s+(.*)' | sed 's/^#s*define __CPU_ARCH__ //g')
}

target=$(get_target_name)

echo "OCD target: '$target'"

if [ "$target" == "stm32f1" ]; then
    openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "init"
elif [ "$target" == "stm32f3" ]; then
    openocd -f interface/stlink.cfg -f target/stm32f3x.cfg -c "init"
elif [ "$target" == "stm32f4" ]; then
    openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "init"
elif [ "$target" == "stm32f7" ]; then
    openocd -f interface/stlink.cfg -f target/stm32f7x.cfg -c "init"
elif [ "$target" == "stm32h7" ]; then
    openocd -f interface/stlink.cfg -f target/stm32h7x.cfg -c "init"
elif [ "$target" == "rpcm0" ]; then
    openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000"
else
    echo "Unknown target: '$target'."
fi
