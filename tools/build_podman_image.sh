#!/usr/bin/env bash

arch=x86_64
if [ $(uname -p) == "arm" ];
    then arch=aarch64;
fi

project_name="dnx-rtos-workspace"

podman build --build-arg ARCHITECTURE=${arch} --build-arg PROJECT_NAME=${project_name} -t ${project_name} .
