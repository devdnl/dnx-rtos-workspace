#!/usr/bin/env bash

project_name="dnx-rtos-workspace"

xhost +
podman run --userns=keep-id:uid=$(id -u),gid=$(id -g) --net=host -it -w $(pwd) -v $(pwd):$(pwd) "localhost/${project_name}:latest"
