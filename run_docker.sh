docker run --net=host --user $(id -u):$(id -g) -it -w /home/developer/project --privileged -v /dev:/dev -v $(pwd):/home/developer/project dnx-rtos-workspace
