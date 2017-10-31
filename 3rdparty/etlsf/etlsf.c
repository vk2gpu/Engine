/*
**  Copyright (c) 2017 Mykhailo Parfeniuk

**  Permission is hereby granted, free of charge, to any person obtaining a copy
**  of this software and associated documentation files (the "Software"), to deal
**  in the Software without restriction, including without limitation the rights
**  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
**  copies of the Software, and to permit persons to whom the Software is
**  furnished to do so, subject to the following conditions:

**  The above copyright notice and this permission notice shall be included in all
**  copies or substantial portions of the Software.

**  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
**  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
**  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
**  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
**  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
**  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
**  SOFTWARE.
*/

#include <etlsf.h>
#include <stddef.h>


#ifndef ETLSF_assert
    #include <assert.h>
    #define ETLSF_assert assert
#endif

#ifndef ETLSF_memset
    #include <memory.h>
    #define ETLSF_memset(ptr, size, value) memset(ptr, value, size)
#endif

#ifndef ETLSF_alloc
    #include <stdlib.h>
    #define ETLSF_alloc(size) malloc(size)
#endif

#ifndef ETLSF_free
    #include <stdlib.h>
    #define ETLSF_free(ptr) free(ptr)
#endif

#ifndef ETLSF_fls
    #if defined (__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) && defined (__GNUC_PATCHLEVEL__)

        static int ETLSF_fls(uint32_t word)
        {
            const int bit = word ? 32 - __builtin_clz(word) : 0;
            return bit - 1;
        }

    #elif defined (_MSC_VER) && (_MSC_VER >= 1400)

        #include <intrin.h>

        #pragma intrinsic(_BitScanReverse)
        #pragma intrinsic(_BitScanForward)

        static int ETLSF_fls(uint32_t word)
        {
            unsigned long index;
            return _BitScanReverse(&index, word) ? index : -1;
        }

    #elif defined (__ARMCC_VERSION)

        static int ETLSF_fls(uint32_t word)
        {
            const int bit = word ? 32 - __clz(word) : 0;
            return bit - 1;
        }

    #else

        static int ETLSF_fls_generic_fls(uint32_t word)
        {
            int bit = 32;

            if (!word) bit -= 1;
            if (!(word & 0xffff0000)) { word <<= 16; bit -= 16; }
            if (!(word & 0xff000000)) { word <<= 8; bit -= 8; }
            if (!(word & 0xf0000000)) { word <<= 4; bit -= 4; }
            if (!(word & 0xc0000000)) { word <<= 2; bit -= 2; }
            if (!(word & 0x80000000)) { word <<= 1; bit -= 1; }

            return bit;
        }

        static int ETLSF_fls(uint32_t word)
        {
            return bit_fls_generic_fls(word) - 1;
        }

    #endif
#endif

#ifndef ETLSF_ffs
    #if defined (__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) && defined (__GNUC_PATCHLEVEL__)

        static int ETLSF_ffs(uint32_t word)
        {
            return __builtin_ffs(word) - 1;
        }

    #elif defined (_MSC_VER) && (_MSC_VER >= 1400)

        #include <intrin.h>

        #pragma intrinsic(_BitScanReverse)
        #pragma intrinsic(_BitScanForward)

        static int ETLSF_ffs(uint32_t word)
        {
            unsigned long index;
            return _BitScanForward(&index, word) ? index : -1;
        }

    #elif defined (__ARMCC_VERSION)

        static int ETLSF_ffs(uint32_t word)
        {
            const unsigned int reverse = word & (~word + 1);
            const int bit = 32 - __clz(reverse);
            return bit - 1;
        }

    #else

        static int ETLSF_fls_generic_ffs(uint32_t word)
        {
            int bit = 32;

            if (!word) bit -= 1;
            if (!(word & 0xffff0000)) { word <<= 16; bit -= 16; }
            if (!(word & 0xff000000)) { word <<= 8; bit -= 8; }
            if (!(word & 0xf0000000)) { word <<= 4; bit -= 4; }
            if (!(word & 0xc0000000)) { word <<= 2; bit -= 2; }
            if (!(word & 0x80000000)) { word <<= 1; bit -= 1; }

            return bit;
        }

        static int ETLSF_ffs(uint32_t word)
        {
            return bit_fls_generic_ffs(word & (~word + 1)) - 1;
        }

    #endif
#endif

enum etlsf_private
{
    /* All allocation sizes and addresses are aligned to 256 bytes. */
    ALIGN_SIZE_LOG2 = 8,
    ALIGN_SIZE = (1 << ALIGN_SIZE_LOG2),
    ALIGN_SIZE_MASK = ALIGN_SIZE - 1,

    /* log2 of number of linear subdivisions of block sizes. */
    SCALE_BIT_COUNT = 4,
    SCALE_VALUE_COUNT = (1 << SCALE_BIT_COUNT),

    /*
    ** We support allocations of sizes up to (1 << MAX_MSB) bits.
    ** However, because we linearly subdivide the second-level lists, and
    ** our minimum size granularity is 4 bytes, it doesn't make sense to
    ** create first-level lists for sizes smaller than SCALE_VALUE_COUNT * 4,
    ** or (1 << (SCALE_BIT_COUNT + 2)) bytes, as there we will be
    ** trying to split size ranges into more slots than we have available.
    ** Instead, we calculate the minimum threshold size, and place all
    ** free_ranges below that size into the 0th first-level list.
    */
    MAX_MSB   = 31,
    MIN_LOG2_EQ_1_BIT = (SCALE_BIT_COUNT + ALIGN_SIZE_LOG2),
    LOG2_COUNT = (MAX_MSB - MIN_LOG2_EQ_1_BIT + 1),

    MIN_LOG2_EQ_1_VALUE = (1 << MIN_LOG2_EQ_1_BIT),

    ALLOC_SIZE_MIN = (size_t)1 << ALIGN_SIZE_LOG2,
    ALLOC_SIZE_MAX = (size_t)1 << MAX_MSB,
};

struct etlsf_range_t
{
    uint16_t  next_phys_index;
    uint16_t  prev_phys_index;

    uint32_t  offset  : 31;
    uint32_t  is_free :  1;

    uint16_t  next_free_index;
    uint16_t  prev_free_index;
};

struct etlsf_private_t
{
    uint32_t size;

    /* Bitmaps for free lists. */
    uint32_t log2_bitset;
    uint32_t scale_bitset[LOG2_COUNT];
    /* Head of free lists. */
    uint16_t free_ranges[LOG2_COUNT][SCALE_VALUE_COUNT];

    uint16_t num_ranges;
    uint16_t next_unused_trailing_index;
    uint16_t first_free_storage_index;
    struct etlsf_range_t storage[1];
};

#define ETLSF_range(id) (arena->storage[id])
#define ETLSF_validate_size(size) ETLSF_assert(size > 0 && size <= ALLOC_SIZE_MAX)
#define ETLSF_validate_index(index) ETLSF_assert(index && (index <= arena->next_unused_trailing_index))

static size_t   arena_total_size(size_t max_allocs);

static uint16_t storage_alloc_range_data(etlsf_t arena);
static void     storage_free_range_data (etlsf_t arena, uint16_t index);

static uint32_t calc_range_size     (etlsf_t arena, uint16_t index);
static void     create_initial_range(etlsf_t arena);
static uint32_t split_range         (etlsf_t arena, uint16_t index, uint32_t size);
static void     merge_ranges        (etlsf_t arena, uint16_t target_index, uint16_t source_index);

static void     freelist_insert_range(etlsf_t arena, uint16_t index);
static void     freelist_remove_range(etlsf_t arena, uint16_t index);
static uint16_t freelist_find_suitable(etlsf_t arena, uint32_t size);

//-------------------------  API implementation  ----------------------------//

etlsf_t etlsf_create(uint32_t size, uint16_t max_allocs)
{
    if (max_allocs == 0 || size < ALLOC_SIZE_MIN || size > ALLOC_SIZE_MAX)
    {
        return 0;
    }

    etlsf_t arena = (etlsf_t)ETLSF_alloc(arena_total_size(max_allocs));

    if (arena)
    {
        ETLSF_memset(arena, sizeof(struct etlsf_private_t), 0); // sets also all lists point to zero block

        arena->size = size;
        arena->num_ranges = max_allocs;

        create_initial_range(arena);
    }

    return arena;
}

void etlsf_destroy(etlsf_t arena)
{
    if (arena)
    {
        ETLSF_free(arena);
    }
}

etlsf_alloc_t etlsf_alloc_range(etlsf_t arena, uint32_t size)
{
    etlsf_alloc_t id = { 0 };

    if (arena && size && size < ALLOC_SIZE_MAX)
    {
        //Align up to min alignment, should not overflow
        uint32_t adjusted_size = (size + ALIGN_SIZE_MASK) & ~ALIGN_SIZE_MASK;

        uint16_t index = freelist_find_suitable(arena, adjusted_size);

        if (index)
        {
            ETLSF_assert(ETLSF_range(index).is_free);
            ETLSF_assert(calc_range_size(arena, index) >= adjusted_size);

            freelist_remove_range(arena, index);

            uint16_t remainder_index = split_range(arena, index, size);

            if (remainder_index)
            {
                freelist_insert_range(arena, remainder_index);
            }

            id.value = index;
        }
    }

    return id;
}

void etlsf_free_range(etlsf_t arena, etlsf_alloc_t id)
{
    if (arena && etlsf_alloc_is_valid(arena, id))
    {
        uint16_t index = id.value;

        //Merge prev block if free
        uint16_t prev_index = ETLSF_range(index).prev_phys_index;
        if (prev_index && ETLSF_range(prev_index).is_free)
        {
            freelist_remove_range(arena, prev_index);
            merge_ranges(arena, prev_index, index);

            index = prev_index;
        }

        //Merge next block if free
        uint16_t next_index = ETLSF_range(index).next_phys_index;
        if (next_index && ETLSF_range(next_index).is_free)
        {
            freelist_remove_range(arena, next_index);
            merge_ranges(arena, index, next_index);
        }

        freelist_insert_range(arena, index);
    }
}

uint32_t etlsf_alloc_size(etlsf_t arena, etlsf_alloc_t id)
{
    return arena && etlsf_alloc_is_valid(arena, id) ? calc_range_size(arena, id.value) : 0;
}

uint32_t etlsf_alloc_offset(etlsf_t arena, etlsf_alloc_t id)
{
    return arena && etlsf_alloc_is_valid(arena, id) ? ETLSF_range(id.value).offset : 0;
}

int etlsf_alloc_is_valid(etlsf_t arena, etlsf_alloc_t id)
{
    uint16_t index = id.value;
    return arena && (index != 0) && (index <= arena->next_unused_trailing_index) && !ETLSF_range(index).is_free;
}

//------------------------------  Arena utils  --------------------------------//

static size_t arena_total_size(size_t max_allocs)
{
    return sizeof(struct etlsf_private_t) + max_allocs * sizeof(struct etlsf_range_t);
}

//----------------------------  Storage utils  --------------------------------//
static uint16_t storage_alloc_range_data(etlsf_t arena)
{
    ETLSF_assert(arena);

    if (arena->first_free_storage_index)
    {
        ETLSF_validate_index(arena->first_free_storage_index);

        uint16_t index = arena->first_free_storage_index;
        arena->first_free_storage_index = ETLSF_range(index).next_phys_index;

        return index;
    }

    if (arena->next_unused_trailing_index < arena->num_ranges)
    {
        return ++arena->next_unused_trailing_index;
    }

    return 0;
}

static void storage_free_range_data(etlsf_t arena, uint16_t index)
{
    ETLSF_assert(arena);
    ETLSF_validate_index(index);
    ETLSF_assert(arena->next_unused_trailing_index && (arena->next_unused_trailing_index < arena->num_ranges));

    if (index == arena->next_unused_trailing_index)
    {
        --arena->next_unused_trailing_index;
    }
    else
    {
        ETLSF_range(index).next_phys_index = arena->first_free_storage_index;
        arena->first_free_storage_index = index;
    }
}

//---------------------------  Physical ranges operations  --------------------------//

static uint32_t calc_range_size(etlsf_t arena, uint16_t index)
{
    ETLSF_assert(arena);
    ETLSF_validate_index(index);

    uint16_t next = ETLSF_range(index).next_phys_index;
    uint32_t size = (next ? ETLSF_range(next).offset : arena->size) - ETLSF_range(index).offset;
    ETLSF_validate_size(size);

    return size;
}

static void create_initial_range(etlsf_t arena)
{
    ETLSF_assert(arena);

    /*
    ** Create the main free block. Offset the start of the block slightly
    ** so that the prev_phys_block field falls outside of the pool -
    ** it will never be used.
    */
    uint16_t index = storage_alloc_range_data(arena);
    ETLSF_range(index).prev_phys_index = 0;
    ETLSF_range(index).next_phys_index = 0;
    ETLSF_range(index).offset = 0;
    freelist_insert_range(arena, index);
}

// returns block created after split
static uint32_t split_range(etlsf_t arena, uint16_t index, uint32_t size)
{
    ETLSF_assert(arena);
    ETLSF_validate_index(index);
    ETLSF_validate_size(size);
    
    uint32_t bsize = calc_range_size(arena, index);

    uint16_t new_index = 0;
    int can_split = bsize >= size + ALLOC_SIZE_MIN;

    if (can_split && (new_index = storage_alloc_range_data(arena)))
    {
        uint16_t next_index = ETLSF_range(index).next_phys_index;
        uint32_t offset = ETLSF_range(index).offset;

        ETLSF_range(index).next_phys_index = new_index;

        ETLSF_range(new_index).offset = offset + size;
        ETLSF_range(new_index).next_phys_index = next_index;
        ETLSF_range(new_index).prev_phys_index = index;

        ETLSF_range(next_index).prev_phys_index = new_index;
    }

    return new_index;
}

static void merge_ranges(etlsf_t arena, uint16_t target_index, uint16_t source_index)
{
    ETLSF_assert(arena);
    ETLSF_validate_index(target_index);
    ETLSF_validate_index(source_index);
    ETLSF_assert(ETLSF_range(target_index).next_phys_index == source_index);

    uint16_t source_next_index = ETLSF_range(source_index).next_phys_index;
    ETLSF_range(target_index).next_phys_index = source_next_index;
    ETLSF_range(source_next_index).prev_phys_index = target_index;

    storage_free_range_data(arena, source_index);
}


//------------------------------  Size utils  -------------------------------//

static inline void size_to_log2_scale(uint32_t size, uint32_t* log2, uint32_t* scale)
{
    assert(log2);
    assert(scale);

    uint32_t msb = ETLSF_fls(size);
    uint32_t non_significant_bits = size < MIN_LOG2_EQ_1_VALUE ? ALIGN_SIZE_LOG2 : msb - SCALE_BIT_COUNT;
    size >>= non_significant_bits;
    *scale = size & (SCALE_VALUE_COUNT - 1);
    size >>= SCALE_BIT_COUNT;
    *log2 = (non_significant_bits - ALIGN_SIZE_LOG2) + (size & 1);
}

static inline void align_size_to_log2_scale(uint32_t size, uint32_t* log2, uint32_t* scale)
{
    assert(log2);
    assert(scale);

    uint32_t msb = ETLSF_fls(size);
    uint32_t non_significant_bits = size < MIN_LOG2_EQ_1_VALUE ? ALIGN_SIZE_LOG2 : msb - SCALE_BIT_COUNT;
    uint32_t non_significant_bits_mask = 0xFFFFFFFF >> (32 - non_significant_bits);
    size += non_significant_bits_mask;
    size >>= non_significant_bits;
    *scale = size & (SCALE_VALUE_COUNT - 1);
    size >>= SCALE_BIT_COUNT;
    *log2 = (non_significant_bits - ALIGN_SIZE_LOG2) + (size & 3);
}

//------------------------------  Free list operations  ------------------------------//

//It is a bug when prev phys block is free
static void freelist_insert_range(etlsf_t arena, uint16_t index)
{
    ETLSF_assert(arena);
    ETLSF_validate_index(index);

    uint32_t log2 = 0, scale = 0;
    uint32_t size = calc_range_size(arena, index);
    size_to_log2_scale(size, &log2, &scale);

    uint16_t next_free_index = arena->free_ranges[log2][scale];
    if (next_free_index)
    {
        ETLSF_validate_index(next_free_index);
        ETLSF_range(next_free_index).prev_free_index = index;
    }

    ETLSF_range(index).prev_free_index = 0;
    ETLSF_range(index).next_free_index = next_free_index;
    ETLSF_range(index).is_free = 1;

    /*
    ** Insert the new block at the head of the list, and mark the first-
    ** and second-level bitmaps appropriately.
    */
    arena->free_ranges[log2][scale] = index;
    arena->log2_bitset     |= (1 << log2);
    arena->scale_bitset[log2] |= (1 << scale);
}

static void freelist_remove_range(etlsf_t arena, uint16_t index)
{
    ETLSF_assert(arena);
    ETLSF_validate_index(index);

    uint32_t log2 = 0, scale = 0;
    uint32_t size = calc_range_size(arena, index);
    size_to_log2_scale(size, &log2, &scale);

    uint16_t prev_index = ETLSF_range(index).prev_free_index;
    uint16_t next_index = ETLSF_range(index).next_free_index;

    if (next_index)
    {
        ETLSF_validate_index(next_index);
        ETLSF_range(next_index).prev_free_index = prev_index;
    }
    if (prev_index)
    {
        ETLSF_validate_index(prev_index);
        ETLSF_range(prev_index).next_free_index = next_index;
    }
    ETLSF_range(index).is_free = 0;

    /* If this block is the head of the free list, set new head. */
    if (arena->free_ranges[log2][scale] == index)
    {
        ETLSF_assert(prev_index == 0);

        arena->free_ranges[log2][scale] = next_index;

        /* If the new head is null, clear the bitmap. */
        if (next_index == 0)
        {
            arena->scale_bitset[log2] &= ~(1 << scale);

            /* If the second bitmap is now empty, clear the log2 bitmap. */
            if (!arena->scale_bitset[log2])
            {
                arena->log2_bitset &= ~(1 << log2);
            }
        }
    }
}

static uint16_t freelist_find_suitable(etlsf_t arena, uint32_t size)
{
    ETLSF_assert(arena);
    ETLSF_validate_size(size);

    uint16_t index = 0;

    if (size)
    {
        uint32_t log2 = 0, scale = 0;
        align_size_to_log2_scale(size, &log2, &scale);

        uint32_t scale_bitset = arena->scale_bitset[log2] & (~0 << scale);
        // if no block search for block with larger size
        if (!scale_bitset)
        {
            const unsigned int log2_bitset = arena->log2_bitset & (~0 << (log2 + 1));
            if (!log2_bitset)
            {
                return 0; // Out of memory
            }

            log2 = ETLSF_ffs(log2_bitset);
            scale_bitset = arena->scale_bitset[log2];
        }

        ETLSF_assert(scale_bitset && "internal error - second level bitmap is null");
        scale = ETLSF_ffs(scale_bitset);

        index = arena->free_ranges[log2][scale];
    }

    return index;
}

#undef ETLSF_assert
#undef ETLSF_memset
#undef ETLSF_alloc
#undef ETLSF_free
#undef ETLSF_fls
#undef ETLSF_ffs
#undef ETLSF_range
#undef ETLSF_validate_index
#undef ETLSF_validate_size
