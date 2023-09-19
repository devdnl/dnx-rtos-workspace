/*******************************************************************************
* author: Daniel Zorychta, danz@jabster.pl
*
* File is based on Freddie Chopin's linker script
*
* compiler: arm-none-eabi-gcc
*******************************************************************************/

/*==============================================================================
format configurations
==============================================================================*/
OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm");
OUTPUT_ARCH(arm);

/*==============================================================================
stacks sizes
==============================================================================*/

/*==============================================================================
available memories definitions
==============================================================================*/
__rom_start = ORIGIN(rom);
__rom_size  = LENGTH(rom);
__rom_end   = __rom_start + __rom_size;

PROVIDE(__rom_start = __rom_start);
PROVIDE(__rom_size  = __rom_size);
PROVIDE(__rom_end   = __rom_end);

/*==============================================================================
entry point
==============================================================================*/
ENTRY(main);

/*==============================================================================
put data in sections
==============================================================================*/
SECTIONS
{
   .text :
   {
      . = ALIGN(4);
      __text_start = .;
      PROVIDE(__text_start = __text_start);

      . = ALIGN(4);
      KEEP(*(.program_descriptor_table));
      . = ALIGN(4);
      *(.text .text.* .gnu.linkonce.t.*);
      . = ALIGN(4);
      *(.glue_7t .glue_7);
      . = ALIGN(4);
      *(.rodata .rodata.* .gnu.linkonce.r.*);

      . = ALIGN(4);
      *(.ARM.extab* .gnu.linkonce.armextab.*);   /* exception unwinding information */
      . = ALIGN(4);
      *(.gcc_except_table);                      /* information used for stack unwinding during exception */
      . = ALIGN(4);
      *(.eh_frame_hdr);                          /* additional information about .ex_frame section */
      . = ALIGN(4);
      *(.eh_frame);                              /* information used for stack unwinding during exception */

      . = ALIGN(4);
      KEEP(*(.init));
      . = ALIGN(4);
      __preinit_array_start = .;
      KEEP(*(.preinit_array));
      . = ALIGN(4);
      __preinit_array_end = .;
      __init_array_start = .;
      KEEP(*(SORT(.init_array.*)));
      . = ALIGN(4);
      KEEP(*(.init_array));
      . = ALIGN(4);
      __init_array_end = .;
      KEEP(*(.fini));
      . = ALIGN(4);
      __fini_array_start = .;
      KEEP(*(.fini_array));
      . = ALIGN(4);
      KEEP(*(SORT(.fini_array.*)));
      . = ALIGN(4);
      __fini_array_end = .;

      . = ALIGN(4);
      __text_end = .;
      PROVIDE(__text_end = __text_end);
   } > rom AT > rom

   . = ALIGN(4);
   __exidx_start = .;
   PROVIDE(__exidx_start = __exidx_start);

   .ARM.exidx :
   {
      *(.ARM.exidx* .gnu.linkonce.armexidx.*);
   } > rom AT > rom                        /* index entries for section unwinding */

   . = ALIGN(4);
   __exidx_end = .;
   PROVIDE(__exidx_end = __exidx_end);

   .stab             0 (NOLOAD) : { *(.stab) }
   .stabstr          0 (NOLOAD) : { *(.stabstr) }

   /* DWARF debug sections.
   * Symbols in the DWARF debugging sections are relative to the beginning
   * of the section so we begin them at 0. */
   /* DWARF 1 */
   .debug               0 : { *(.debug) }
   .line                0 : { *(.line) }

   /* GNU DWARF 1 extensions */
   .debug_srcinfo       0 : { *(.debug_srcinfo) }
   .debug_sfnames       0 : { *(.debug_sfnames) }

   /* DWARF 1.1 and DWARF 2 */
   .debug_aranges       0 : { *(.debug_aranges) }
   .debug_pubnames      0 : { *(.debug_pubnames) }

   /* DWARF 2 */
   .debug_info          0 : { *(.debug_info .gnu.linkonce.wi.*) }
   .debug_abbrev        0 : { *(.debug_abbrev) }
   .debug_line          0 : { *(.debug_line) }
   .debug_frame         0 : { *(.debug_frame) }
   .debug_str           0 : { *(.debug_str) }
   .debug_loc           0 : { *(.debug_loc) }
   .debug_macinfo       0 : { *(.debug_macinfo) }

   /* SGI/MIPS DWARF 2 extensions */
   .debug_weaknames     0 : { *(.debug_weaknames) }
   .debug_funcnames     0 : { *(.debug_funcnames) }
   .debug_typenames     0 : { *(.debug_typenames) }
   .debug_varnames      0 : { *(.debug_varnames) }

   .note.gnu.arm.ident  0 : { KEEP(*(.note.gnu.arm.ident)) }
   .ARM.attributes      0 : { KEEP(*(.ARM.attributes)) }
   /DISCARD/            : { *(.note.GNU-stack) }
}

PROVIDE(__text_size  = __text_end  - __text_start);
PROVIDE(__exidx_size = __exidx_end - __exidx_start);

/*******************************************************************************
END OF FILE
*******************************************************************************/
