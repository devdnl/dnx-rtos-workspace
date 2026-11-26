#!/usr/bin/env python3

import subprocess
import os
import sys
import platform
import hashlib
from pathlib import Path

image_name = "dnx-rtos-workspace"
cwd = Path.cwd()
short_hash =  hashlib.sha1(str(cwd).encode()).hexdigest()[:8]
container_name = Path.cwd().name + '-' + short_hash
cont_platform = "docker"
pwd = os.path.abspath(os.getcwd())

# definition of the workspace synchronization of the project (tools/workspace_sync.py)
WORKSPACE_SYNC_CONFIG = os.path.join("utils", "workspace_sync.toml")

def run(cmd):
    subprocess.run(cmd, check=False)

def get_output(cmd):
    return subprocess.check_output(cmd, text=True).strip()

def sync_mounts():
    """Directories of the workspace synchronization outside the project (workspace
    and kernel given by path): list of (host directory, container directory).
    A directory outside the project is mounted at the path seen by the script
    in the container (target of a link mounted at the place of the link), a
    link in the project is resolved in the container by mounting its target at
    the same path."""
    try:
        import tomllib
        with open(WORKSPACE_SYNC_CONFIG, "rb") as f:
            cfg = tomllib.load(f)
    except (ImportError, OSError, ValueError):
        return []

    paths = [cfg.get("workspace", {}).get("path")]
    kernel = cfg.get("kernel", {})
    if kernel.get("source") == "path":
        paths.append(kernel.get("path"))

    base = os.path.dirname(os.path.realpath(WORKSPACE_SYNC_CONFIG))
    project = os.path.realpath(pwd)
    mounts = []
    for path in paths:
        if not isinstance(path, str):
            continue
        path = os.path.expanduser(path)
        dst = os.path.normpath(os.path.join(base, path))
        src = os.path.realpath(dst)
        if not os.path.isdir(src) or (src + os.sep).startswith(project + os.sep):
            continue
        if (dst + os.sep).startswith(project + os.sep):
            dst = src
        covered = any(src == s or (src.startswith(s + os.sep) and dst == d + src[len(s):])
                      for s, d in mounts)
        if not covered:
            mounts.append((src, dst))
    return mounts

def mounted_dirs():
    try:
        out = get_output([cont_platform, "inspect", "-f",
                          "{{range .Mounts}}{{.Destination}}\n{{end}}", container_name])
    except (subprocess.CalledProcessError, OSError):
        return None
    return set(out.split())

def containers_list():
    result = subprocess.run(
        [cont_platform, "ps", "-a", "--format", "{{.Names}}"],
        capture_output=True,
        text=True
    )
    return result.stdout.splitlines()

exists = container_name in containers_list()

if exists:
    mounted = mounted_dirs()
    if mounted is not None:
        for src, dst in sync_mounts():
            if dst not in mounted:
                print(f"[warning] {dst} is not mounted in container {container_name}: "
                      f"remove the container ({cont_platform} rm {container_name}) to create it again")

if not exists:
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

    for src, dst in sync_mounts():
        print(f"Mount {src} at {dst} (workspace synchronization)")
        cmd += ["-v", f"{src}:{dst}"]

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
