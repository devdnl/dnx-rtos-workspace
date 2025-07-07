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
project_flags_file="$4/project/project_flags.h"
appset_section_address=$(cat $project_flags_file | grep __PROJ_APPSET_TABLE_ADDRESS__ | sed  's/.*__PROJ_APPSET_TABLE_ADDRESS__\s*//')
appset_section_size=$(cat $project_flags_file | grep __PROJ_APPSET_TABLE_SIZE__ | sed  's/.*__PROJ_APPSET_TABLE_SIZE__\s*//')

if [ "$appset_section_address" = "" ]; then
    echo "Unable to get appset section address! Error in $project_flags_file: __PROJ_APPSET_TABLE_ADDRESS__"
    exit 1
fi

if [ "$appset_section_size" = "" ]; then
    echo "Unable to get appset section size! Error in $project_flags_file: __PROJ_APPSET_TABLE_SIZE__"
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

    if [ "$1" == "" ] || [ "$2" == "" ] || [ "$3" == "" ] || [ "$4" == "" ]; then
        echo "Usage: $args <path_to_scan> <output_path> <appfilter> <config-path>"
        exit 1
    fi
}

#-------------------------------------------------------------------------------
# @brief  Gets program list in the current directory. Folders are interpreted as
#         programs. Files are ignored.
# @param  path to scan
# @return program list
#-------------------------------------------------------------------------------
function get_program_list()
{
    if [ "$2" == "*" ]; then
        echo $(ls -F "$1/programs" | grep -P '/|@' | grep -Pv '#' | sed 's/\///g' | sed 's/@//g')
    else
        echo "$2"
    fi
}

#-------------------------------------------------------------------------------
# @brief  Gets program list in the current directory. Folders are interpreted as
#         libraries. Files are ignored.
# @param  path to scan
# @return program list
#-------------------------------------------------------------------------------
function get_library_list()
{
    echo $(ls -F "$1/libs" | grep -P '/|@' | grep -Pv '#' | sed 's/\///g' | sed 's/@//g')
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

    if [ "$1" == "*" ]; then
        echo 'CSRC_PROGRAMS += ../../$(GEN_PROG_DIR)/program_registration.c'
    fi

    echo 'HDRLOC_PROGRAMS += ../programs'
    for prog in $program_list; do
        echo '-include $(APP_PRG_LOC)/'"$prog"'/Makefile.in'
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
        echo '-include $(APP_LIB_LOC)/'"$lib"'/Makefile.in'
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
    echo '    _PROGMAN_EXTERN_C const size_t __builtin_app_##_name_##_gs__;\'
    echo '    _PROGMAN_EXTERN_C const size_t __builtin_app_##_name_##_ss__;\'
    echo '    _PROGMAN_EXTERN_C const char *const *const __builtin_app_##_name_##_pref_ram__;\'
    echo '    _PROGMAN_EXTERN_C int __builtin_app_##_name_##_main(int, char**);'
    echo ''

    echo '#define _PROGRAM_ENTRY(_name_)\'
    echo '    {.name          = #_name_,\'
    echo '     .main          = __builtin_app_##_name_##_main,\'
    echo '     .globals_size  = &__builtin_app_##_name_##_gs__,\'
    echo '     .stack_depth   = &__builtin_app_##_name_##_ss__,\'
    echo '     .pref_ram      = &__builtin_app_##_name_##_pref_ram__}'
    echo ''

    echo 'typedef struct {'
    echo '    const char     *name;'
    echo '    const size_t   *globals_size;'
    echo '    const size_t   *stack_depth;'
    echo '    const char     *const *const *pref_ram;'
    echo '    int (*main)(int, char**);'
    echo '} _program_entry_t;'
    echo ''
    echo 'typedef struct {'
    echo '        uint32_t magic;'
    echo '        uint32_t number_of_programs;'
    echo '        const _program_entry_t program_entry[];'
    echo '} _program_table_desc_t;'
    echo ''

    for prog in $program_list; do
        echo "_IMPORT_PROGRAM($prog);"
    done
    echo ''

    echo 'const _program_table_desc_t _program_table_desc __attribute__((section (".program_descriptor_table"))) = {'
    echo "        .magic = 0,"
    echo "        .number_of_programs = $(echo $program_list | wc -w),"
    echo "        .program_entry = {"
    for prog in $program_list; do
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

    if [ "$1" == "*" ]; then
        echo "   rom (rx)  : org = $appset_section_address, len = $appset_section_size"
    else
        echo "   app (rwx)  : org = 0, len = $appset_section_size"
    fi
    echo "}"

    if [ "$1" == "*" ]; then
        echo "INCLUDE memory_layout.ld"
    else
        echo "INCLUDE memory_layout_standalone.ld"
    fi
}

#-------------------------------------------------------------------------------
# @brief  Script main function
# @param  1st argument
# @return None
#-------------------------------------------------------------------------------
function main()
{
    check_args "$1" "$2" "$3" "$4"

    program_list=$(get_program_list "$1" "$3")

    create_makefile_programs "$3" > "$2/$Makefile_name"
    create_program_registration_file "$3" > "$2/$program_registration_file_name"
    create_linker_scirpt_file "$3" > "$2/$linker_script_file_name"

    library_list=$(get_library_list "$1")
    create_makefile_libs >> "$2/$Makefile_name"
}

main "$1" "$2" "$3" "$4"

