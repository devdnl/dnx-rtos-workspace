SEARCH_DIR(.);
SEARCH_DIR(src/cpu/stm32h7/ld);
MEMORY
{
	axisram (rwx) : org = 0x24000000, len = 512k
	bkp     (rw)  : org = 0x38800000, len = 4k
	dtcm    (rw)  : org = 0x20000000, len = 128k
	flash   (rx)  : org = 0x08080000, len = 512k
	itcm    (rwx) : org = 0x00000000, len = 64k
	sram1   (rwx) : org = 0x30000000, len = 128k
	sram2   (rwx) : org = 0x30020000, len = 128k
	sram3   (rwx) : org = 0x30040000, len = 32k
	sram4   (rwx) : org = 0x38000000, len = 64k
}
INCLUDE common.ld
