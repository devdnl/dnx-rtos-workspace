#!/usr/bin/env bash

docker run --net=host --user $(id -u):$(id -g) -it -w /home/developer/dnx-rtos-workspace --privileged -v /dev:/dev -v $(pwd):/home/developer/dnx-rtos-workspace dnx-rtos-workspace
