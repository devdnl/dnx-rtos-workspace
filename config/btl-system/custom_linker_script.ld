SEARCH_DIR(.);
SEARCH_DIR(src/cpu/stm32f1/ld);
MEMORY
{
	ram (rwx) : org = 0x20000000, len = 20k
	rom (rx)  : org = 0x08000000, len = 128k
}
INCLUDE common.ld
