# Workspace tools

This directory belongs to the dnx RTOS workspace: the core of the Makefile
(`workspace.mk`: macros, help, clean; example targets included by the project
`Makefile`: `target_app.mk` application, `target_btl.mk` bootloader,
`target_sim.mk` POSIX simulator, `target_test_sim.mk` tests in the simulator),
build bases of appsets (`appset/`) and romfs (`romfs/`), drvset tools
(`drvset/`), scripts used by `Makefile` (BSP import/export, configuration,
flashing, OpenOCD, terminal, linker script generator, Docker image), simulator
tests (`simtest.py`), the board panel of the simulator (`simpanel.py`) and
synchronization of a project with the workspace (`workspace_sync.py`,
definition example `workspace_sync.toml`).

Files are updated together with the workspace, so do not put scripts and
tools of your project here: use `utils/` for them.
