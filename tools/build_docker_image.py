#!/usr/bin/env python3

import os
import platform
import subprocess

machine = platform.machine().lower()

if "arm" in machine or "aarch64" in machine:
    arch = "aarch64"
else:
    arch = "x86_64"

project_name = "dnx-rtos-workspace"
project_name_lower = project_name.lower()

env = os.environ.copy()
env["DOCKER_BUILDKIT"] = "1"

cmd = [
    "docker", "build",
    "--build-arg", f"ARCHITECTURE={arch}",
    "--build-arg", f"PROJECT_NAME={project_name_lower}",
    "-t", project_name_lower,
    "."
]

print("Running:", " ".join(cmd))

result = subprocess.run(cmd, env=env)

exit(result.returncode)
