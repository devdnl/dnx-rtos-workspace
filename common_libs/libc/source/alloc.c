/*==============================================================================
@file    alloc.c

@author  Daniel Zorychta

@brief   Memory allocate functions.

@note    Copyright (C) 2021 Daniel Zorychta <daniel.zorychta@gmail.com>

         This program is free software; you can redistribute it and/or modify
         it under the terms of the GNU General Public License as published by
         the Free Software Foundation and modified by the dnx RTOS exception.

         NOTE: The modification  to the GPL is  included to allow you to
               distribute a combined work that includes dnx RTOS without
               being obliged to provide the source  code for proprietary
               components outside of the dnx RTOS.

         The dnx RTOS  is  distributed  in the hope  that  it will be useful,
         but WITHOUT  ANY  WARRANTY;  without  even  the implied  warranty of
         MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
         GNU General Public License for more details.

         Full license text is available on the following file: doc/license.txt.


==============================================================================*/

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include <dnx/misc.h>
#include "common.h"

/*==============================================================================
  Local macros
==============================================================================*/
#define ALLOC_BLOCK_SIZE 1024
#define get_heap()       (((_libc_app_ctx_t *)(_libc_appctx_get()))->_heap)

/*==============================================================================
  Local object types
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/
extern void *memset(void *s, int c, size_t n);
extern void *memcpy(void *dest, const void *src, size_t n);

/*==============================================================================
  Local objects
==============================================================================*/

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  External objects
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Set heap allocation size. It is minimal memory block size that will
 *         be requested from system to be used as heap memory.
 *
 * @param  alloc_size           allocation size
 *
 * @return One of errno value.
 */
//==============================================================================
int set_heap_allocation_size(size_t alloc_size)
{
        return _libc_heap_set_allocation_size(get_heap(), alloc_size);
}

//==============================================================================
/**
 * @brief Function allocates memory block.
 *
 * The malloc() function allocates <i>size</i> bytes and returns a pointer to the
 * allocated memory. The memory is not initialized.  If size is 0, then malloc()
 * returns either @ref NULL.
 *
 * @param size      size bytes to allocate
 *
 * @exception | @ref ENOMEM
 *
 * @return Returns a pointer to the allocated memory, which is suitably aligned
 * for any built-in type.  On error, these functions return @ref NULL.  The
 * @ref NULL pointer may also be returned by a successful call to function with
 * a <i>size</i> of zero.
 */
//==============================================================================
void *_libc_malloc(size_t size)
{
        _libc_heap_t *heap = get_heap();
        if (!heap) {
                heap       = _libc_heap_new(ALLOC_BLOCK_SIZE);
                get_heap() = heap;
        }

        return _libc_heap_malloc(heap, size);
}

//==============================================================================
/**
 * @brief Function allocates memory block.
 *
 * The calloc() function allocates memory for an array of <i>n</i> elements
 * of <i>size</i> bytes each and returns a pointer to the allocated memory.
 * The memory is set to zero.
 *
 * @param n         number of elements
 * @param size      size of elements
 *
 * @exception | @ref ENOMEM
 *
 * @return Returns a pointer to the allocated memory, which is suitably aligned
 * for any built-in type.  On error, these functions return @ref NULL. The
 * @ref NULL pointer may also be returned by a successful call to function with
 * a <i>n</i> or <i>size</i> of zero.
 */
//==============================================================================
void *_libc_zalloc(size_t size)
{
        void *ptr = _libc_malloc(size);
        if (ptr) {
                memset(ptr, 0, size);
        }
        return ptr;
}

//==============================================================================
/**
 * @brief Function frees allocated memory block.
 *
 * The free() function frees the memory space pointed to by <i>ptr</i>, which
 * must have been returned by a previous call to malloc(), calloc(),
 * realloc(). If <i>ptr</i> is @ref NULL, no operation is performed.
 *
 * @param ptr       pointer to memory space to be freed
 *
 * @exception | @ref ESRCH
 * @exception | @ref ENOENT
 * @exception | @ref EFAULT
 * @exception | @ref EINVAL
 */
//==============================================================================
void _libc_free(void *ptr)
{
        if (ptr) {
                _libc_heap_t *heap = get_heap();
                _libc_heap_free(heap, ptr);
        } else {
                errno = EINVAL;
        }
}

//==============================================================================
/**
 * @brief Function changes the size of allocated memory block.
 *
 * The realloc() function changes the size of the memory block pointed
 * to by <i>ptr</i> to size bytes. The contents will be unchanged in the range
 * from the start of the region up to the minimum of the old and new
 * sizes.  If the new size is larger than the old size, the added memory
 * will not be initialized.  If <i>ptr</i> is @ref NULL, then the call is equivalent
 * to malloc(size), for all values of <i>size</i>; if <i>size</i> is equal to
 * zero, and <i>ptr</i> is not @ref NULL, then the call is equivalent to free(ptr).
 * Unless <i>ptr</i> is @ref NULL, it must have been returned by an earlier call to
 * malloc(), calloc() or realloc().  If the area pointed to was moved, a
 * free(ptr) is done.
 *
 * @param ptr       pointer to memory space
 * @param size      size of new memory space
 *
 * @return Pointer to allocated memory or NULL on error/free.
 */
//==============================================================================
void *_libc_realloc(void *ptr, size_t size)
{
        void *mem = NULL;

        if (size) {
                mem = _libc_malloc(size);
                if (mem) {
                        if (ptr) {
                                _libc_heap_t *heap = get_heap();
                                size_t len = _libc_heap_get_block_size(heap, ptr);
                                memcpy(mem, ptr, min(len, size));
                                _libc_free(ptr);
                        }
                } else {
                        _libc_free(ptr);
                }
        } else {
                _libc_free(ptr);
        }

        return mem;
}

/*==============================================================================
  End of file
==============================================================================*/
