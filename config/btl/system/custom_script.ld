SEARCH_DIR(.);
SEARCH_DIR(src/cpu/stm32f1/ld);

/* example custom linker script for STM32F103RB */

MEMORY
{
   rom (rx)  : org = 0x08000000, len = 128k
   ram (rwx) : org = 0x20000000, len = 20k
}

INCLUDE common.ld
