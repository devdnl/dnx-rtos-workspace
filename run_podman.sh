#!/usr/bin/env bash

project_name="dnx-rtos-workspace"

xhost +
podman run --userns=keep-id:uid=$(id -u),gid=$(id -g) --net=host -it -w $(pwd) -e DISPLAY=$DISPLAY -v $(pwd):$(pwd):Z,U -v /etc/localtime:/etc/localtime:ro -v /tmp/.X11-unix:/tmp/.X11-unix --rm "localhost/${project_name}:latest"
