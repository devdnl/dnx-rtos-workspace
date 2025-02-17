#!/usr/bin/env bash

arch=x86_64
if [ $(uname -p) == "arm" ];
    then arch=aarch64;
fi

DOCKER_BUILDKIT=1 docker build --build-arg ARCHITECTURE=${arch} -t dnx-rtos-workspace .
