#!/usr/bin/env bash

project_name="dnx-rtos-workspace"

xhost +
docker run --net=host --user $(id -u):$(id -g) -it -w $(pwd) --privileged -e DISPLAY=$DISPLAY -v /dev:/dev -v $(pwd):$(pwd) -v /etc/localtime:/etc/localtime:ro -v /tmp/.X11-unix:/tmp/.X11-unix --rm ${project_name}
