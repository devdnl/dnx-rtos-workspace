#!/usr/bin/env bash
#-------------------------------------------------------------------------------
#
# NOTE: script add programs and libraries to the system.
#
#-------------------------------------------------------------------------------

# configuration
Makefile_name="Makefile"
program_registration_file_name="program_registration.c"
linker_script_file_name="layout.ld"
project_flags_file="$5/project/project_flags.h"
appset_rom_section_address=$(cat $project_flags_file | grep __PROJ_APPSET_TABLE_ADDRESS__ | sed  's/.*__PROJ_APPSET_TABLE_ADDRESS__\s*//')
appset_rom_section_size=$(cat $project_flags_file | grep __PROJ_APPSET_TABLE_SIZE__ | sed  's/.*__PROJ_APPSET_TABLE_SIZE__\s*//')
appset_ram_section_address=$(cat $project_flags_file | grep __PROJ_APPSET_RAM_ADDRESS__ | sed  's/.*__PROJ_APPSET_RAM_ADDRESS__\s*//')
appset_ram_section_size=$(cat $project_flags_file | grep __PROJ_APPSET_RAM_SIZE__ | sed  's/.*__PROJ_APPSET_RAM_SIZE__\s*//')

if [ "$appset_rom_section_address" = "" ]; then
    echo "Unable to get appset section address! Error in $project_flags_file: __PROJ_APPSET_TABLE_ADDRESS__"
    exit 1
fi

if [ "$appset_rom_section_size" = "" ]; then
    echo "Unable to get appset section size! Error in $project_flags_file: __PROJ_APPSET_TABLE_SIZE__"
    exit 1
fi

if [ "$appset_ram_section_address" = "" ]; then
    echo "Unable to get appset RAM address! Error in $project_flags_file: __PROJ_APPSET_RAM_ADDRESS__"
    exit 1
fi

if [ "$appset_ram_section_size" = "" ]; then
    echo "Unable to get appset RAM size! Error in $project_flags_file: __PROJ_APPSET_RAM_SIZE__"
    exit 1
fi

# variables
program_list=

#-------------------------------------------------------------------------------
# @brief  Check incoming arguments
# @param  arguments
# @return None
#-------------------------------------------------------------------------------
function check_args()
{
    local args="$1"

    if [ "$1" == "" ] || [ "$2" == "" ] || [ "$3" == "" ] || [ "$4" == "" ] || [ "$5" == "" ]; then
        echo "Usage: $args <path_to_scan> <output_path> <app-include> <app-exclude> <config-path>"
        exit 1
    fi
}

#-------------------------------------------------------------------------------
# @brief  Remove entries from input list by using exclude list.
# @return new list
#-------------------------------------------------------------------------------
filter_list() {
    local input="$1"
    local exclude="$2"
    local result=""

    for item in $input; do
        skip=0
        for remove in $exclude; do
            i="${item##*/}"
            i="${i%%.*}"
            [[ "$i" == "$remove" ]] && skip=1 && break
        done
        [[ $skip -eq 0 ]] && result="$result $item"
    done

    echo "$result"
}

#-------------------------------------------------------------------------------
# @brief  Gets program list in the current directory. Folders are interpreted as
#         programs. Files are ignored.
# @param  $1    path to scan
# @return program list
#-------------------------------------------------------------------------------
function get_program_list()
{
    shopt -s nullglob

    for d in $1/programs/*/; do
        name="${d%/}"
        [[ "$name" == \#* ]] && continue
        [ -f "$d/Makefile.in" ] && echo "${name}"
    done

    for f in $1/programs/*.mk; do
        [[ "$f" == \#* ]] && continue
        echo "${f}"
    done
}

#-------------------------------------------------------------------------------
# @brief  Gets program list in the current directory. Folders are interpreted as
#         libraries. Files are ignored.
# @param  path to scan
# @return program list
#-------------------------------------------------------------------------------
function get_library_list()
{
    shopt -s nullglob

    for d in $1/libs/*/; do
        name="${d%/}"
        [[ "$name" == \#* ]] && continue
        [ -f "$d/Makefile.in" ] && echo "${name}"
    done

    for f in $1/libs/*.mk; do
        [[ "$f" == \#* ]] && continue
        echo "${f}"
    done
}

#-------------------------------------------------------------------------------
# @brief  Creates empty Makefile
# @param  None
# @return None
#-------------------------------------------------------------------------------
function create_makefile_programs()
{
    echo '# Makefile for GNU make - file generated at build process'
    echo ''

    echo 'CSRC_PROGRAMS += ../../$(GEN_PROG_DIR)/program_registration.c'

    echo 'HDRLOC_PROGRAMS += ../programs'
    for prog in $program_list; do
        if [ -d "$prog" ]; then
            echo '-include $(APP_PRG_LOC)/'"${prog##*/}"'/Makefile.in'
        else
            echo '-include $(APP_PRG_LOC)/'"${prog##*/}"
        fi
    done
}

#-------------------------------------------------------------------------------
# @brief  Creates empty Makefile
# @param  None
# @return None
#-------------------------------------------------------------------------------
function create_makefile_libs()
{
    echo ""
    echo '# libraries'
    echo 'HDRLOC_LIB += ../libs'

    for lib in $library_list; do
        if [ -d "$lib" ]; then
            echo '-include $(APP_LIB_LOC)/'"${lib##*/}"'/Makefile.in'
        else
            echo '-include $(APP_LIB_LOC)/'"${lib##*/}"
        fi
    done
}

#-------------------------------------------------------------------------------
# @brief  Creates program registration file
# @param  None
# @return None
#-------------------------------------------------------------------------------
function create_program_registration_file()
{
    echo '// file generated automatically at build process'
    echo ''
    echo '#include <stdint.h>'
    echo '#include <stddef.h>'
    echo ''

    echo '#define _PROGMAN_EXTERN_C extern'
    echo ''

    echo '#define _IMPORT_PROGRAM(_name_)\'
    echo '        _PROGMAN_EXTERN_C const size_t __builtin_app_##_name_##_gs__;\'
    echo '        _PROGMAN_EXTERN_C const size_t __builtin_app_##_name_##_ss__;\'
    echo '        _PROGMAN_EXTERN_C const char *const *const __builtin_app_##_name_##_pref_ram__;\'
    echo '        _PROGMAN_EXTERN_C int __builtin_app_##_name_##_main(int, char**);'
    echo ''

    echo '#define _PROGRAM_ENTRY(_name_)\'
    echo '{\'
    echo '        .name = #_name_,\'
    echo '        .main = __builtin_app_##_name_##_main,\'
    echo '        .globals_size = &__builtin_app_##_name_##_gs__,\'
    echo '        .stack_depth = &__builtin_app_##_name_##_ss__,\'
    echo '        .pref_ram = &__builtin_app_##_name_##_pref_ram__,\'
    echo '}'
    echo ''        

    echo 'typedef struct {'
    echo '        const char *name;'
    echo '        const size_t *globals_size;'
    echo '        const size_t *stack_depth;'
    echo '        const char *const *const *pref_ram;'
    echo '        int (*main)(int, char**);'
    echo '} _program_entry_t;'
    echo ''
    echo 'typedef struct {'
    echo '        const uint32_t magic;'
    echo '        const uint32_t number_of_programs;'
    echo '        const void *data_init_start;'
    echo '        void *data_start;'
    echo '        const uint32_t data_size;'
    echo '        void *bss_start;'
    echo '        const uint32_t bss_size;'
    echo '        const _program_entry_t program_entry[];'
    echo '} _program_table_desc_t;'
    echo ''

    for prog in $program_list; do
        prog="${prog##*/}"
        prog="${prog%%.*}"
        echo "_IMPORT_PROGRAM($prog);"
    done
    echo ''

    echo 'extern void *__data_init_start;'
    echo 'extern void *__data_start;'
    echo 'extern void *__data_size;'
    echo 'extern void *__bss_start;'
    echo 'extern void *__bss_size;'
    echo ''

    echo 'const _program_table_desc_t _program_table_desc __attribute__((section (".program_descriptor_table"))) = {'
    echo "        .magic = 0x32565341, // ASV2"
    echo "        .number_of_programs = $(echo $program_list | wc -w),"
    echo "        .data_init_start = ((void *)&__data_init_start),"
    echo "        .data_start = ((void *)&__data_start),"
    echo "        .data_size = ((size_t)&__data_size),"
    echo "        .bss_start = ((void *)&__bss_start),"
    echo "        .bss_size = ((size_t)&__bss_size),"
    echo "        .program_entry = {"
    for prog in $program_list; do
        prog="${prog##*/}"
        prog="${prog%%.*}"
        echo "                _PROGRAM_ENTRY($prog),"
    done
    echo "        }"
    echo "};"
    echo ''

    echo "// function never called"
    echo "int main(void) {"
    echo "        _program_table_desc.program_entry->main(0, NULL);"
    echo "        return 0;"
    echo "}"
}

#-------------------------------------------------------------------------------
# @brief  Creates linker script
# @param  None
# @return None
#-------------------------------------------------------------------------------
function create_linker_scirpt_file()
{
    echo "SEARCH_DIR(.);"
    echo "SEARCH_DIR(tools);"

    echo "MEMORY"
    echo "{"
    echo "   rom (rx)  : org = $appset_rom_section_address, len = $appset_rom_section_size"
    echo "   ram (rwx) : org = $appset_ram_section_address, len = $appset_ram_section_size"
    echo "}"

    echo "INCLUDE memory_layout.ld"
}

#-------------------------------------------------------------------------------
# @brief  Script main function
# @param  1st argument
# @return None
#-------------------------------------------------------------------------------
function main()
{
    check_args "$1" "$2" "$3" "$4" "$5"

    if [ "$3" == "*" ]; then
        program_list=$(get_program_list "$1")
        program_list=$(filter_list "${program_list}" "$4")
    else
        program_list=$(filter_list "$3" "$4")
    fi

    create_makefile_programs "$3" > "$2/$Makefile_name"
    create_program_registration_file "$3" > "$2/$program_registration_file_name"
    create_linker_scirpt_file "$3" > "$2/$linker_script_file_name"

    library_list=$(get_library_list "$1")
    create_makefile_libs >> "$2/$Makefile_name"
}

main "$1" "$2" "$3" "$4" "$5"

