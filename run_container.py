#!/usr/bin/env python3

import subprocess
import os
import sys
import platform
from pathlib import Path

image_name = "dnx-rtos-workspace"
container_name = Path.cwd().name
cont_platform = "docker"
pwd = os.path.abspath(os.getcwd())

def run(cmd):
    subprocess.run(cmd, check=False)

def get_output(cmd):
    return subprocess.check_output(cmd, text=True).strip()

def containers_list():
    result = subprocess.run(
        [cont_platform, "ps", "-a", "--format", "{{.Names}}"],
        capture_output=True,
        text=True
    )
    return result.stdout.splitlines()

if container_name not in containers_list():
    print("Creating container...")

    system = platform.system()
    display_env = os.environ.get("DISPLAY", "")

    cmd = [cont_platform, "run", "-it", "--privileged"]

    if system == "Linux":
        run(["xhost", "+"])
        uid = get_output(["id", "-u"])
        gid = get_output(["id", "-g"])

        cmd += [
            "--net=host",
            "--user", f"{uid}:{gid}",
            "-v", "/dev:/dev",
            "-v", "/tmp/.X11-unix:/tmp/.X11-unix",
            "-v", "/etc/localtime:/etc/localtime:ro",
        ]

    elif system == "Darwin":
        display_env = "host.docker.internal:0"
        cmd += [
            "-v", "/dev:/dev",
            "-v", "/etc/localtime:/etc/localtime:ro",
        ]

    elif system == "Windows":
        pwd = pwd.replace("\\", "/")
        display_env = os.environ.get("DISPLAY", "host.docker.internal:0")
        cmd += [
        ]

    cmd += [
        "-w", pwd,
        "-v", f"{pwd}:{pwd}",
        "--name", container_name,
    ]

    if display_env:
        cmd += ["-e", f"DISPLAY={display_env}"]

    cmd += [image_name]

    run(cmd)

elif len(sys.argv) > 1:
    run([cont_platform, "start", container_name])
    run([cont_platform, "exec", container_name] + sys.argv[1:])
    run([cont_platform, "exec", container_name, "sync"])
    run([cont_platform, "kill", container_name])

else:
    run([cont_platform, "start", "-ai", container_name])
