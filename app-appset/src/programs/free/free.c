/*==============================================================================
@file   free.c

@author Daniel Zorychta

@brief  Show used/free memory

        MIT License

        Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>

        Permission is hereby granted, free of charge, to any person obtaining
        a copy of this software and associated documentation files (the
        "Software"), to deal in the Software without restriction, including
        without limitation the rights to use, copy, modify, merge, publish,
        distribute, sublicense, and/or sell copies of the Software, and to
        permit persons to whom the Software is furnished to do so, subject to
        the following conditions:

        The above copyright notice and this permission notice shall be included
        in all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
        OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
        THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
        OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
        ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
        OTHER DEALINGS IN THE SOFTWARE.

==============================================================================*/

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dnx/os.h>
#include <dnx/vt100.h>
#include <sys/mount.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/

/*==============================================================================
  Local types, enums definitions
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(free, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief Cat main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        (void)argc;

        uint32_t drv_count = get_number_of_drivers();
        int32_t *modmem = malloc(drv_count * sizeof(int32_t));
        if (!modmem) {
                perror(NULL);
                return EXIT_FAILURE;
        }

        memstat_t sysmem;
        get_memory_usage_details(&sysmem);

        for (uint module = 0; module < drv_count; module++) {
                get_driver_memory_usage(module, &modmem[module]);
        }


        u32_t m_free = sysmem.free_memory;
        u32_t m_used = sysmem.used_memory;
        u32_t m_size = sysmem.memory_size;
        u32_t m_perc = ((u64_t)m_used * 1000) / m_size;

        printf("Total: %u\n", m_size);
        printf("Free: %u\n", m_free);
        printf("Used: %u\n", m_used);
        printf("Memory usage: %u.%u%%\n", m_perc / 10, m_perc % 10);

        if (strcmp(argv[1], "-d") == 0) {
                printf("\nDetailed memory usage:\n");
                printf("  Kernel: %d\n", sysmem.kernel_memory_usage);
                printf("  Filesystems: %d\n", sysmem.filesystems_memory_usage);
                printf("  Modules: %d\n", sysmem.modules_memory_usage);
                printf("  Network: %d\n", sysmem.network_memory_usage);
                printf("  Programs: %d\n", sysmem.programs_memory_usage);
                printf("  Shared: %d\n", sysmem.shared_memory_usage);
                printf("  Cached: %d\n", sysmem.cached_memory_usage);
                printf("  Static: %d\n\n", sysmem.static_memory_usage);

                printf("Detailed modules memory usage:\n");
                for (uint module = 0; module < drv_count; module++) {
                        printf("  %s: %d\n",
                               get_driver_name(module),
                               modmem[module]);
                }

                printf("\nDetailed region information:\n");
                printf("N - no heap, C - cacheable, D - DMA\n");
                printf("Flags   Address       Size  Heap-size Heap-usage Name\n");

                size_t idx = 0;
                mem_region_info_t info;
                while (get_memory_region_info(idx++, &info) == 0) {
                        printf("  %c%c%c %08Xh %10lu %10lu %10lu \"%s\"\n",
                               info.flags & MEM_FLAG__NOHEAP ? 'N' : '-',
                               info.flags & MEM_FLAG__CACHE ? 'C' : '-',
                               info.flags & MEM_FLAG__DMA ? 'D' : '-',
                               info.address,
                               info.size,
                               info.heap_size,
                               info.heap_usage,
                               info.name_ref);
                }
        }

        free(modmem);

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
