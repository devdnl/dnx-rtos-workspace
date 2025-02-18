#!/usr/bin/env bash

project_name="$(basename $(readlink -m $(pwd)))"

docker run --net=host --user $(id -u):$(id -g) -it -w /home/developer/${project_name} --privileged -v /dev:/dev -v $(pwd):/home/developer/${project_name} ${project_name}
