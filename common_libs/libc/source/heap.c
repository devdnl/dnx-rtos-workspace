/*==============================================================================
@file   heap.c

@author Daniel Zorychta

@brief  This file support dynamic memory management.

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
/*
* Copyright (c) 2001-2004 Swedish Institute of Computer Science.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote products
*    derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
* SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
*
* This file is part of the lwIP TCP/IP stack.
*
* Author: Adam Dunkels <adam@sics.se>
*         Simon Goldschmidt
*
==============================================================================*/

#pragma GCC optimize("Ofast")

/*==============================================================================
  Include files
==============================================================================*/
#include "heap.h"
#include <stdio.h>
#include <dnx/misc.h>
#include <stdatomic.h>
#include <unistd.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define HEAP_OVERFLOW_CHECK     __LIBC_HEAP_OVERFLOW_CHECK__
#define HEAP_SANITY_CHECK       __LIBC_HEAP_SANITY_CHECK__
#define HEAP_OVF_REGION_SIZE    __LIBC_HEAP_OVF_REGION_SIZE__
#define HEAP_EMPTY_REGION_VALUE __LIBC_HEAP_EMPTY_REGION_VALUE__

#define HEAP_ALIGN      sizeof(int)
#define HEAP_BLOCK_SIZE 4

/**
 * Calculate memory size for an aligned buffer - returns the next highest
 * multiple of MEM_ALIGNMENT (e.g. MEM_ALIGN_SIZE(3) and
 * MEM_ALIGN_SIZE(4) will both yield 4 for MEM_ALIGNMENT == 4).
 */
#define MEM_ALIGN_SIZE(size) (((size) + HEAP_ALIGN - 1) & ~(HEAP_ALIGN - 1))

/** some alignment macros: we define them here for better source code layout */
#define BLOCK_MIN_SIZE_ALIGNED MEM_ALIGN_SIZE(HEAP_BLOCK_SIZE)
#define SIZEOF_STRUCT_MEM      MEM_ALIGN_SIZE(sizeof(struct mem))

#if HEAP_OVERFLOW_CHECK
#define MEM_SANITY_REGION_BEFORE_ALIGNED HEAP_OVF_REGION_SIZE
#define MEM_SANITY_REGION_AFTER_ALIGNED  HEAP_OVF_REGION_SIZE
#else
#define MEM_SANITY_REGION_BEFORE_ALIGNED 0
#define MEM_SANITY_REGION_AFTER_ALIGNED  0
#endif

#define MEM_SANITY_OFFSET MEM_SANITY_REGION_BEFORE_ALIGNED

#define HEAP_ASSERT(_cond, _msg, _fail_var) \
        if (!(_cond)) {                     \
                _fail_var = true;           \
                printf(_msg);               \
        }

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef struct mem_heap {
        struct mem_heap *next;
        struct mem *ram_end;
        struct mem volatile *lfree;
        size_t size;
        size_t used;
        uint_aligned8_t ram_start[];
} mem_heap_t;

struct _libc_heap {
        atomic_flag futex;
        mem_heap_t *mem;
        size_t alloc_block_size;
};

/**
 * The heap is made up as a list of structs of this type.
 * This does not have to be aligned since for getting its size,
 * we only use the macro SIZEOF_STRUCT_MEM, which automatically alignes.
 */
struct mem {
        size_t next; /**< index (-> ram[next]) of the next struct      */
        size_t prev; /**< index (-> ram[prev]) of the previous struct  */
        u8_t used;   /**< 1: this area is used; 0: this area is unused */
#if HEAP_OVERFLOW_CHECK
        size_t user_size; /**< user size used in overflow check */
#endif
};

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void plug_holes(mem_heap_t *heap, struct mem *mem);
static bool mem_heap_check_consistency(mem_heap_t *heap);

/*==============================================================================
  Local object definitions
==============================================================================*/

/*==============================================================================
  Exported object definitions
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Immediate abort running program.
 */
//==============================================================================
static void abort_now(void)
{
        _libc_syscall(_LIBC_SYS_PROCESSABORT);
}

//==============================================================================
/**
 * @brief  Memory allocate syscall.
 *
 * @param  size         block size
 *
 * @return Pointer to memory or NULL.
 */
//==============================================================================
static void *sys_malloc(size_t size)
{
        void *mem;
        int err = _libc_syscall(_LIBC_SYS_MALLOC, &size, &mem);
        return err ? NULL : mem;
}

//==============================================================================
/**
 * @brief  Memory allocate syscall.
 *
 * @param  size         block size
 *
 * @return Pointer to memory or NULL.
 */
//==============================================================================
static void *sys_zalloc(size_t size)
{
        void *mem;
        int err = _libc_syscall(_LIBC_SYS_ZALLOC, &size, &mem);
        return err ? NULL : mem;
}

//==============================================================================
/**
 * @brief  Memory free syscall.
 *
 * @param  ptr          memory block
 */
//==============================================================================
static void sys_free(void *ptr)
{
        _libc_syscall(_LIBC_SYS_FREE, ptr);
}

//==============================================================================
/**
 * @brief  Protect heap access.
 *
 * @param  heap         heap instance
 */
//==============================================================================
static void protect(_libc_heap_t *heap)
{
        while (atomic_flag_test_and_set_explicit(&heap->futex, memory_order_acquire)) {
                msleep(0);
        }
}

//==============================================================================
/**
 * @brief  Unprotect heap access.
 *
 * @param  heap         heap instance
 */
//==============================================================================
static void unprotect(_libc_heap_t *heap)
{
        atomic_flag_clear_explicit(&heap->futex, memory_order_release);
}

//==============================================================================
/**
 * @brief  Function convert heap pointer to block representation.
 *
 * @param  heap         heap instance
 * @param  ptr          pointer to convert
 *
 * @return Pointer to block.
 */
//==============================================================================
static struct mem *ptr_to_mem(mem_heap_t *heap, size_t ptr)
{
        return (struct mem *)(void *)&heap->ram_start[ptr];
}

//==============================================================================
/**
 * @brief  Function convert block representation to heap pointer.
 *
 * @param  heap         heap instance
 * @param  mem          block representation
 *
 * @return Heap pointer.
 */
//==============================================================================
static size_t mem_to_ptr(mem_heap_t *heap, volatile void *mem)
{
        return (size_t)((u8_t *)mem - heap->ram_start);
}

//==============================================================================
/**
 * @brief  "Plug holes" by combining adjacent empty struct mems.
 *         After this function is through, there should not exist
 *         one empty struct mem pointing to another empty struct mem.
 *         This assumes access to the heap is protected by the calling function
 *         already.
 *
 * @param  heap       heap object
 * @param  mem        this points to a struct mem which just has been freed
 */
//==============================================================================
static void plug_holes(mem_heap_t *heap, struct mem *mem)
{
        /* plug hole forward */
        struct mem *nmem = ptr_to_mem(heap, mem->next);

        if (mem != nmem && nmem->used == 0 && (u8_t *)nmem != (u8_t *)heap->ram_end) {
                /* if mem->next is unused and not end of ram, combine mem and mem->next */
                if (heap->lfree == nmem) {
                        heap->lfree = mem;
                }

                mem->next = nmem->next;
                if (nmem->next != heap->size) {
                        ptr_to_mem(heap, nmem->next)->prev = mem_to_ptr(heap, mem);
                }

#if HEAP_OVERFLOW_CHECK
                memset(nmem, HEAP_EMPTY_REGION_VALUE, MEM_ALIGN_SIZE(sizeof(*nmem)));
#endif
        }

        /* plug hole backward */
        struct mem *pmem = ptr_to_mem(heap, mem->prev);

        if (pmem != mem && pmem->used == 0) {
                /* if mem->prev is unused, combine mem and mem->prev */
                if (heap->lfree == mem) {
                        heap->lfree = pmem;
                }

                pmem->next = mem->next;
                if (mem->next != heap->size) {
                        ptr_to_mem(heap, mem->next)->prev = mem_to_ptr(heap, pmem);
                }

#if HEAP_OVERFLOW_CHECK
                memset(mem, HEAP_EMPTY_REGION_VALUE, MEM_ALIGN_SIZE(sizeof(*mem)));
#endif
        }
}

//==============================================================================
/**
 * @brief  Function check if memory block representation is valid.
 *
 * @param  heap         heap instance
 * @param  mem          memory block representation
 *
 * @return 0: link invalid; 1: link valid.
 */
//==============================================================================
static int link_valid(mem_heap_t *heap, struct mem *mem)
{
        size_t rmem_idx = mem_to_ptr(heap, mem);
        struct mem *nmem = ptr_to_mem(heap, mem->next);
        struct mem *pmem = ptr_to_mem(heap, mem->prev);

        if ((mem->next > heap->size) || (mem->prev > heap->size) ||
                ((mem->prev != rmem_idx) && (pmem->next != rmem_idx)) ||
                ((nmem != heap->ram_end) && (nmem->prev != rmem_idx))) {

                return 0;

        } else {
                return 1;
        }
}

#if HEAP_OVERFLOW_CHECK
//==============================================================================
/**
 * @brief  Function check block overflow.
 *
 * @param  p            pointer to examine
 * @param  size         allocation size
 */
//==============================================================================
static void overflow_check_raw(void *p, size_t size)
{
        u8_t *m = (u8_t *)p - MEM_SANITY_REGION_BEFORE_ALIGNED;
        for (u16_t k = 0; k < MEM_SANITY_REGION_BEFORE_ALIGNED; k++) {
                if (m[k] != 0xFA) {
                        printf("HEAP: block underflow @ %p, pos %u\n", p, k);
                        abort_now();
                        break;
                }
        }

        m = (u8_t *)p + size;
        for (u16_t k = 0; k < MEM_SANITY_REGION_AFTER_ALIGNED; k++) {
                if (m[k] != 0xFB) {
                        printf("HEAP: block overflow @ %p, pos %u\n", p, k);
                        abort_now();
                        break;
                }
        }
}

//==============================================================================
/**
 * @brief  Function initialize overflow parameters.
 *
 * @param  p            block
 * @param  size         alloc size
 */
//==============================================================================
static void mem_overflow_init_raw(void *p, size_t size)
{
        u8_t *m = (u8_t *)p - MEM_SANITY_REGION_BEFORE_ALIGNED;
        memset(m, 0xFA, MEM_SANITY_REGION_BEFORE_ALIGNED);

        m = (u8_t *)p + size;
        memset(m, 0xFB, MEM_SANITY_REGION_AFTER_ALIGNED);
}
#endif

//==============================================================================
/**
 * @brief  Function initialize overflow parameters of memory block representation.
 *
 * @param  mem          memory block
 * @param  user_size    allocated size
 */
//==============================================================================
static void overflow_init_element(struct mem *mem, size_t user_size)
{
#if HEAP_OVERFLOW_CHECK
        void *p = (u8_t *)mem + SIZEOF_STRUCT_MEM + MEM_SANITY_OFFSET;
        mem->user_size = user_size;
        mem_overflow_init_raw(p, user_size);
#else
        (void)mem;
        (void)user_size;
#endif
}

//==============================================================================
/**
 * @brief  Function check if block overflow occurred.
 *
 * @param  mem          memory block
 */
//==============================================================================
static void overflow_check_element(struct mem *mem)
{
#if HEAP_OVERFLOW_CHECK
        void *p = (u8_t *)mem + SIZEOF_STRUCT_MEM + MEM_SANITY_OFFSET;
        overflow_check_raw(p, mem->user_size);
#else
        (void)mem;
#endif
}

//==============================================================================
/**
 * @brief  Create new memory heap.
 *
 * @param  size          memory size
 *
 * @return One of errno value.
 */
//==============================================================================
static mem_heap_t *mem_heap_new(size_t size)
{
        if (size == 0) {
                return NULL;
        }

        size = MEM_ALIGN_SIZE(size + (2 * (SIZEOF_STRUCT_MEM + MEM_SANITY_OFFSET)));

        mem_heap_t *heap = sys_zalloc(sizeof(*heap) + size);
        if (not heap) {
                return NULL;
        }

#if HEAP_OVERFLOW_CHECK
        memset(heap, HEAP_EMPTY_REGION_VALUE, sizeof(*heap) + size);
        memset(heap, 0, sizeof(*heap));
#endif

        heap->used = 0;
        heap->size = MEM_ALIGN_SIZE(size - SIZEOF_STRUCT_MEM - MEM_SANITY_OFFSET);

        struct mem *mem = (struct mem *)heap->ram_start;
        mem->next = heap->size;
        mem->prev = 0;
        mem->used = 0;

        heap->ram_end = ptr_to_mem(heap, heap->size);
        heap->ram_end->used = 1;
        heap->ram_end->next = heap->size;
        heap->ram_end->prev = heap->size;

        heap->lfree = (struct mem *)heap->ram_start;

#if HEAP_SANITY_CHECK
        mem_heap_check_consistency(heap);
#endif

        return heap;
}

//==============================================================================
/**
 * @brief  Delete memory heap.
 *
 * @param  mem          memory heap
 *
 * @return NULL.
 */
//==============================================================================
static mem_heap_t *mem_heap_delete(mem_heap_t *mem)
{
        sys_free(mem);
        return NULL;
}

//==============================================================================
/**
 * @brief  Put a struct mem back on the heap
 *
 * @param  heap         heap object
 * @param  rmem         is the data portion of a struct mem as returned by a previous
 *                      call to _heap_alloc()
 */
//==============================================================================
static void mem_heap_free(mem_heap_t *heap, void *rmem)
{
        if (!heap || !rmem || !heap->size) {
                return;
        }

        if ((((uintptr_t)rmem) & (HEAP_ALIGN - 1)) != 0) {
                printf("HEAP: free: unaligned pointer\n");
                abort_now();
                return;
        }

        struct mem *mem = (struct mem *)(void *)((u8_t *)rmem - (SIZEOF_STRUCT_MEM + MEM_SANITY_OFFSET));
        if ((u8_t *)mem < heap->ram_start || (u8_t *)rmem + BLOCK_MIN_SIZE_ALIGNED > (u8_t *)heap->ram_end) {
                printf("HEAP: free: illegal pointer\n");
                abort_now();
                return;
        }

        overflow_check_element(mem);

        if (mem->used > 1) {
                printf("HEAP: free: invalid usage indicator\n");
                abort_now();
                return;
        }

        if (!mem->used) {
                printf("HEAP: free: block unused\n");
                abort_now();
                return;
        }

        if (!link_valid(heap, mem)) {
                printf("HEAP: free: invalid link\n");
                abort_now();
                return;
        }

        mem->used = 0;

        if (mem < heap->lfree) {
                /* the newly freed struct is now the lowest */
                heap->lfree = mem;
        }

        size_t blksize = (mem->next - (size_t)(((u8_t *)mem - heap->ram_start)));

#if HEAP_SANITY_CHECK and HEAP_OVERFLOW_CHECK
        u8_t *ptr = (u8_t *)mem + SIZEOF_STRUCT_MEM;
        memset(ptr, HEAP_EMPTY_REGION_VALUE,
                MEM_SANITY_REGION_BEFORE_ALIGNED + mem->user_size + MEM_SANITY_REGION_AFTER_ALIGNED);
#endif

        heap->used -= blksize;

        plug_holes(heap, mem);

#if HEAP_SANITY_CHECK
        mem_heap_check_consistency(heap);
#endif
}

//==============================================================================
/**
 * @brief  Allocate a block of memory with a minimum of 'size' bytes.
 *         Note that the returned value will always be aligned (as defined by MEM_ALIGNMENT).
 *
 * @param  heap         heap object
 * @param  size_in      is the minimum size of the requested block in bytes.

 * @return Pointer to allocated memory or NULL if no free memory was found.
 */
//==============================================================================
static void *mem_heap_malloc(mem_heap_t *heap, size_t size_in)
{
        size_t ptr, ptr2, size;
        struct mem *mem, *mem2;
        size_t used;

        if (!heap || size_in == 0 || !heap->size) {
                return NULL;
        }

        /* Expand the size of the allocated memory region so that we can adjust for alignment. */
        size = MEM_ALIGN_SIZE(size_in);
        if (size < BLOCK_MIN_SIZE_ALIGNED) {
                /* every data block must be at least BLOCK_MIN_SIZE_ALIGNED long */
                size = BLOCK_MIN_SIZE_ALIGNED;
        }

        size += MEM_SANITY_REGION_BEFORE_ALIGNED + MEM_SANITY_REGION_AFTER_ALIGNED;

        if (size > heap->size || size < size_in) {
                return NULL;
        }

        /*
         * Scan through the heap searching for a free block that is big enough,
         * beginning with the lowest free block.
         */
        for (ptr = mem_to_ptr(heap, heap->lfree); ptr < heap->size - size;
                ptr = ptr_to_mem(heap, ptr)->next) {

                mem = ptr_to_mem(heap, ptr);

                if ((!mem->used) && (mem->next - (ptr + SIZEOF_STRUCT_MEM)) >= size) {
                        /*
                         * mem is not used and at least perfect fit is possible:
                         * mem->next - (ptr + SIZEOF_STRUCT_MEM) gives us the 'user data size' of mem
                         */

                        if (mem->next - (ptr + SIZEOF_STRUCT_MEM) >=
                                (size + SIZEOF_STRUCT_MEM + BLOCK_MIN_SIZE_ALIGNED)) {
                                /* (in addition to the above, we test if another struct mem
                                 * (SIZEOF_STRUCT_MEM) containing
                                 * at least BLOCK_MIN_SIZE_ALIGNED of data also fits in the 'user
                                 * data space' of 'mem')
                                 * -> split large block, create empty remainder,
                                 * remainder must be large enough to contain BLOCK_MIN_SIZE_ALIGNED data: if
                                 * mem->next - (ptr + (2*SIZEOF_STRUCT_MEM)) == size,
                                 * struct mem would fit in but no data between mem2 and mem2->next
                                 * @todo we could leave out BLOCK_MIN_SIZE_ALIGNED. We would create an empty
                                 *       region that couldn't hold data, but when mem->next gets freed,
                                 *       the 2 regions would be combined, resulting in more free memory
                                 */
                                ptr2 = ptr + SIZEOF_STRUCT_MEM + size;

                                /* create mem2 struct */
                                mem2 = ptr_to_mem(heap, ptr2);
                                mem2->used = 0;
                                mem2->next = mem->next;
                                mem2->prev = ptr;
                                /* and insert it between mem and mem->next */
                                mem->next = ptr2;
                                mem->used = 1;

                                if (mem2->next != heap->size) {
                                        ptr_to_mem(heap, mem2->next)->prev = ptr2;
                                }

                                used = (size + SIZEOF_STRUCT_MEM);
                        } else {
                                /* (a mem2 struct does no fit into the user data space of
                                 *  mem and mem->next will always
                                 * be used at this point: if not we have 2 unused structs
                                 * in a row, plug_holes should have
                                 * take care of this).
                                 * -> near fit or excact fit: do not split, no mem2 creation
                                 * also can't move mem->next directly behind mem, since mem->next
                                 * will always be used at this point!
                                 */
                                mem->used = 1;

                                used = (mem->next - mem_to_ptr(heap, mem));
                        }

                        if (mem == heap->lfree) {
                                volatile struct mem *cur = heap->lfree;

                                /* Find next free block after mem and update lowest free pointer */
                                while (cur->used && cur != heap->ram_end) {
                                        cur = ptr_to_mem(heap, cur->next);
                                }

                                heap->lfree = cur;
                        }

                        heap->used += used;

                        overflow_init_element(mem, size_in);

#if HEAP_SANITY_CHECK
                        mem_heap_check_consistency(heap);
#endif

                        return (u8_t *)mem + SIZEOF_STRUCT_MEM + MEM_SANITY_OFFSET;
                }
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Function check heap consistency.
 *
 * @param  heap         heap instance
 *
 * @return Return true if sanity correct, otherwise false.
 */
//==============================================================================
static bool mem_heap_check_consistency(mem_heap_t *heap)
{
        bool fail = false;

        HEAP_ASSERT(MEM_ALIGN_SIZE(heap->size) == heap->size, "HEAP: invalid heap size alignment\n", fail);
        if (fail) {
                abort_now();
                return false;
        }

        /* begin with first element here */
        struct mem *mem = (struct mem *)heap->ram_start;

        HEAP_ASSERT((mem->used == 0) || (mem->used == 1), "HEAP: element used invalid\n", fail);
        if (fail) {
                abort_now();
                return false;
        }

        u8_t last_used = mem->used;

        HEAP_ASSERT(mem->prev == 0, "HEAP: element prev ptr not valid\n", fail);
        if (fail) {
                abort_now();
                return false;
        }

        HEAP_ASSERT(mem->next <= heap->size, "HEAP: element next ptr invalid\n", fail);
        if (fail) {
                abort_now();
                return false;
        }

        HEAP_ASSERT(MEM_ALIGN_SIZE(ptr_to_mem(heap, mem->next) == ptr_to_mem(heap, mem->next)),
                "HEAP: element next ptr unaligned\n", fail);
        if (fail) {
                abort_now();
                return false;
        }

        /* check all elements before the end of the heap */
        size_t max = heap->size / SIZEOF_STRUCT_MEM;

        for (mem = ptr_to_mem(heap, mem->next); ((u8_t *)mem > heap->ram_start) && (mem < heap->ram_end);
                mem = ptr_to_mem(heap, mem->next)) {

#if HEAP_OVERFLOW_CHECK
                if (mem->used) {
                        overflow_check_element(mem);
                } else {
                        u8_t *ptr = (u8_t *)mem + SIZEOF_STRUCT_MEM;
                        size_t blksize = (mem->next - (size_t)(((u8_t *)mem - heap->ram_start))) -
                                SIZEOF_STRUCT_MEM;

                        for (size_t i = 0; i < blksize; i++) {
                                HEAP_ASSERT(ptr[i] == HEAP_EMPTY_REGION_VALUE,
                                        "HEAP: empty element overwritten\n", fail);
                                if (fail) {
                                        abort_now();
                                        return false;
                                }
                        }
                }
#endif

                HEAP_ASSERT(MEM_ALIGN_SIZE((uintptr_t)mem) == (uintptr_t)mem, "HEAP: element unaligned\n",
                        fail);
                if (fail) {
                        abort_now();
                        return false;
                }

                HEAP_ASSERT(mem->prev <= heap->size, "HEAP: element prev ptr invalid\n", fail);
                if (fail) {
                        abort_now();
                        return false;
                }

                HEAP_ASSERT(mem->next <= heap->size, "HEAP: element next ptr invalid\n", fail);
                if (fail) {
                        abort_now();
                        return false;
                }

                HEAP_ASSERT(MEM_ALIGN_SIZE(ptr_to_mem(heap, mem->prev) == ptr_to_mem(heap, mem->prev)),
                        "HEAP: element prev ptr unaligned\n", fail);
                if (fail) {
                        abort_now();
                        return false;
                }

                HEAP_ASSERT(MEM_ALIGN_SIZE(ptr_to_mem(heap, mem->next) == ptr_to_mem(heap, mem->next)),
                        "HEAP: element next ptr unaligned\n", fail);
                if (fail) {
                        abort_now();
                        return false;
                }

                if (last_used == 0) {
                        /* 2 unused elements in a row? */
                        HEAP_ASSERT(mem->used == 1, "HEAP: 2 element unused in row?\n", fail);
                        if (fail) {
                                abort_now();
                                return false;
                        }
                } else {
                        HEAP_ASSERT((mem->used == 0) || (mem->used == 1), "HEAP: element used invalid\n",
                                fail);
                        if (fail) {
                                abort_now();
                                return false;
                        }
                }

                HEAP_ASSERT(link_valid(heap, mem), "HEAP: element link invalid\n", fail);
                if (fail) {
                        abort_now();
                        return false;
                }

                /* used/unused altering */
                last_used = mem->used;

                HEAP_ASSERT(--max > 0, "HEAP: sanity: dead loop\n", fail);
                if (fail) {
                        abort_now();
                        return false;
                }
        }

        HEAP_ASSERT(mem == ptr_to_mem(heap, heap->size), "HEAP: end ptr sanity\n", fail);
        if (fail) {
                abort_now();
                return false;
        }

        HEAP_ASSERT(mem->used == 1, "HEAP: element used invalid\n", fail);
        if (fail) {
                abort_now();
                return false;
        }

        HEAP_ASSERT(mem->prev == heap->size, "HEAP: element prev ptr invalid\n", fail);
        if (fail) {
                abort_now();
                return false;
        }

        HEAP_ASSERT(mem->next == heap->size, "HEAP: element next ptr invalid\n", fail);
        if (fail) {
                abort_now();
                return false;
        }

        return true;
}

//==============================================================================
/**
 * @brief  Function return free heap
 *
 * @param  heap         heap object
 *
 * @return Free heap value
 */
//==============================================================================
static size_t mem_heap_get_free(mem_heap_t *heap)
{
        return (heap->size - heap->used);
}

//==============================================================================
/**
 * @brief  Function return used heap
 *
 * @param  heap         heap object
 *
 * @return Use heap value
 */
//==============================================================================
static size_t mem_heap_get_used(mem_heap_t *heap)
{
        return heap->used;
}

//==============================================================================
/**
 * @brief  Function return heap size
 *
 * @param  heap         heap object
 *
 * @return Heap size
 */
//==============================================================================
static size_t mem_heap_get_size(mem_heap_t *heap)
{
        if (heap->size) {
                return heap->size + SIZEOF_STRUCT_MEM;
        } else {
                return 0;
        }
}

//==============================================================================
/**
 * @brief  Function return size of selected block
 *
 * @param  heap     heap object
 * @param  rmem     memory block
 *
 * @return Block size, 0 on error
 */
//==============================================================================
static size_t mem_heap_get_block_size(mem_heap_t *heap, void *rmem)
{
        if (!heap || !rmem || !heap->size) {
                return 0;
        }

        if ((((uintptr_t)rmem) & (HEAP_ALIGN - 1)) != 0) {
                printf("HEAP: blksize: unaligned pointer\n");
                return 0;
        }

        struct mem *mem = (struct mem *)(void *)((u8_t *)rmem - (SIZEOF_STRUCT_MEM + MEM_SANITY_OFFSET));
        if ((u8_t *)mem < heap->ram_start || (u8_t *)rmem + BLOCK_MIN_SIZE_ALIGNED > (u8_t *)heap->ram_end) {
                printf("HEAP: blksize: illegal pointer\n");
                return 0;
        }

        if (mem->used > 1) {
                printf("HEAP: blksize: invalid usage indicator\n");
                return 0;
        }

        if (!mem->used) {
                printf("HEAP: blksize: block unused\n");
                return 0;
        }

        if (!link_valid(heap, mem)) {
                printf("HEAP: blksize: invalid link\n");
                return 0;
        }

        return (mem->next - (size_t)(((u8_t *)mem - heap->ram_start)));
}

//==============================================================================
/**
 * @brief  Create new heap instance.
 *
 * @param  size          minimal memory allocation size
 *
 * @return One of errno value.
 */
//==============================================================================
_libc_heap_t *_libc_heap_new(size_t alloc_block_size)
{
        if (alloc_block_size == 0) {
                return NULL;
        }

        _libc_heap_t *heap = sys_malloc(sizeof(*heap));
        if (not heap) {
                return NULL;
        }

        atomic_flag_clear(&heap->futex);
        heap->alloc_block_size = alloc_block_size;
        heap->mem = NULL;

        return heap;
}

//==============================================================================
/**
 * @brief  Delete heap instance.
 *
 * @param  heap         heap instance.
 *
 * @return NULL.
 */
//==============================================================================
_libc_heap_t *_libc_heap_delete(_libc_heap_t *heap)
{
        if (not heap) {
                return NULL;
        }

        mem_heap_t *mem = heap->mem;

        while (mem) {
                mem_heap_t *next = mem->next;
                mem_heap_delete(mem);
                mem = next;
        }

        sys_free(heap);

        return NULL;
}

//==============================================================================
/**
 * @brief  Free memory block from heap.
 *
 * @param  heap         heap instance
 * @param  ptr          pointer
 *
 * @return NULL.
 */
//==============================================================================
void *_libc_heap_free(_libc_heap_t *heap, void *ptr)
{
        if (not heap or not ptr) {
                return NULL;
        }

        mem_heap_t *mem = heap->mem;
        mem_heap_t *mem_prev = NULL;

        protect(heap);

        while (mem) {
                if ((ptr >= cast(void *, mem->ram_start)) and (ptr <= cast(void *, mem->ram_end))) {

                        mem_heap_free(mem, ptr);

                        if (mem_heap_get_used(mem) == 0) {

                                if (mem_prev) {
                                        mem_prev->next = mem->next;
                                } else {
                                        heap->mem = mem->next;
                                }

                                mem_heap_delete(mem);
                        }

                        break;
                }

                mem_prev = mem;
                mem = mem->next;
        }

        unprotect(heap);

        return NULL;
}

//==============================================================================
/**
 * @brief  Allocate memory block.
 *
 * @param  heap         heap instance
 * @param  size         block size to allocate
 *
 * @return On success pointer to block, otherwise NULL.
 */
//==============================================================================
void *_libc_heap_malloc(_libc_heap_t *heap, size_t size)
{
        if (not heap or not size) {
                return NULL;
        }

        void *ptr = NULL;
        mem_heap_t *mem = heap->mem;
        mem_heap_t *mem_last = heap->mem;

        protect(heap);

        while (mem) {
                if (mem_heap_get_free(mem) >= size) {
                        ptr = mem_heap_malloc(mem, size);
                        if (ptr) {
                                break;
                        }
                }

                mem_last = mem;
                mem = mem->next;
        }

        if (not ptr) {
                size_t heap_size = ((size / heap->alloc_block_size) + 1) * heap->alloc_block_size;

                if (mem_last) {
                        mem_last->next = mem_heap_new(heap_size);
                        mem = mem_last->next;
                } else {
                        heap->mem = mem_heap_new(heap_size);
                        mem = heap->mem;
                }

                if (mem) {
                        ptr = mem_heap_malloc(mem, size);
                }
        }

        unprotect(heap);

        return ptr;
}

//==============================================================================
/**
 * @brief  Get free heap space (all memory heaps).
 *
 * @param  heap         heap instance
 *
 * @return Free bytes.
 */
//==============================================================================
size_t _libc_heap_get_free(_libc_heap_t *heap)
{
        if (not heap) {
                return 0;
        }

        size_t free = 0;
        mem_heap_t *mem = heap->mem;

        protect(heap);

        while (mem) {
                free += mem_heap_get_free(mem);
                mem = mem->next;
        }

        unprotect(heap);

        return free;
}

//==============================================================================
/**
 * @brief  Get used heap space (all memory heaps).
 *
 * @param  heap         heap instance
 *
 * @return Used bytes.
 */
//==============================================================================
size_t _libc_heap_get_used(_libc_heap_t *heap)
{
        if (not heap) {
                return 0;
        }

        size_t used = 0;
        mem_heap_t *mem = heap->mem;

        protect(heap);

        while (mem) {
                used += mem_heap_get_used(mem);
                mem = mem->next;
        }

        unprotect(heap);

        return used;
}

//==============================================================================
/**
 * @brief  Get heap size (all memory heaps).
 *
 * @param  heap         heap instance
 *
 * @return Sum in bytes.
 */
//==============================================================================
size_t _libc_heap_get_size(_libc_heap_t *heap)
{
        if (not heap) {
                return 0;
        }

        size_t size = 0;
        mem_heap_t *mem = heap->mem;

        protect(heap);

        while (mem) {
                size += mem_heap_get_size(mem);
                mem = mem->next;
        }

        unprotect(heap);

        return size;
}

//==============================================================================
/**
 * @brief  Get block size.
 *
 * @param  heap         heap instance
 * @param  ptr          block to examine
 *
 * @return Block size.
 */
//==============================================================================
size_t _libc_heap_get_block_size(_libc_heap_t *heap, void *ptr)
{
        if (not heap or not ptr) {
                return 0;
        }

        size_t size = 0;
        mem_heap_t *mem = heap->mem;

        protect(heap);

        while (mem) {
                if ((ptr >= cast(void *, mem->ram_start)) and (ptr < cast(void *, mem->ram_end))) {
                        size = mem_heap_get_block_size(mem, ptr);
                        break;
                }

                mem = mem->next;
        }

        unprotect(heap);

        return size;
}

//==============================================================================
/**
 * @brief  Check heap consistency (all memory heaps).
 *
 * @param  heap         heap instance
 *
 * @return True if consistent, otherwise false.
 */
//==============================================================================
bool _libc_heap_is_consistent(_libc_heap_t *heap)
{
        if (not heap) {
                return false;
        }

        bool consistent = false;
        mem_heap_t *mem = heap->mem;

        protect(heap);

        while (mem) {
                consistent = mem_heap_check_consistency(mem);
                if (not consistent) {
                        break;
                }
                mem = mem->next;
        }

        unprotect(heap);

        return consistent;
}

//==============================================================================
/**
 * @brief  Set heap allocation size. It is minimal memory block size that will
 *         be requested from system to be used as heap memory.
 *
 * @param  alloc_block_size     allocation block size
 *
 * @return One of errno value.
 */
//==============================================================================
int _libc_heap_set_allocation_size(_libc_heap_t *heap, size_t alloc_block_size)
{
        if (not heap) {
                return EINVAL;
        }

        protect(heap);
        heap->alloc_block_size = min(alloc_block_size, 64);
        unprotect(heap);

        return 0;
}

/*==============================================================================
  End of file
==============================================================================*/
