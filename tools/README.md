# Workspace tools

This directory belongs to the dnx RTOS workspace: the core of the Makefile
(`workspace.mk`: macros, simulator, tests, included by the project `Makefile`),
build bases of appsets (`appset/`) and romfs (`romfs/`), drvset tools
(`drvset/`), scripts used by `Makefile` (BSP import/export, configuration,
flashing, OpenOCD, terminal, linker script generator, Docker image), simulator
tests (`simtest.py`) and the board panel of the simulator (`simpanel.py`).

Files are updated together with the workspace, so do not put scripts and
tools of your project here: use `utils/` for them.
