#!/usr/bin/env bash

arch=x86_64
if [ $(uname -p) == "arm" ];
    then arch=aarch64;
fi

#project_name="$(basename $(readlink -m "$(pwd)/.."))"
project_name="dnx-rtos-workspace"

DOCKER_BUILDKIT=1 docker build --build-arg ARCHITECTURE=${arch} --build-arg PROJECT_NAME=${project_name} -t ${project_name} .
