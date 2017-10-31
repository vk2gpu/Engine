#pragma once

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

/*
**  External allocator based on Two Level Segregated Fit memory allocator
**  from http://tlsf.baisoku.org
*/

#include <stdint.h>

//Allocator is not threadsafe
//Supports spaces up to 2^30
//Supports arbitrary alignments up to 256
//Supports up to 65535 allocation max
//Uses 16 bytes per possible allocation

#ifdef __cplusplus
extern "C" {
#endif

struct etlsf_private_t;
typedef struct etlsf_private_t*  etlsf_t;
typedef struct { uint16_t value; } etlsf_alloc_t;

static const etlsf_alloc_t ETLSF_INVALID_ID = { 0 };

etlsf_t  etlsf_create (uint32_t size, uint16_t max_allocs);
void     etlsf_destroy(etlsf_t arena);

// All allocations are always aligned up to 256 bytes
etlsf_alloc_t etlsf_alloc_range(etlsf_t arena, uint32_t size);
void          etlsf_free_range (etlsf_t arena, etlsf_alloc_t id);

/* Returns internal block size, not original request size */
uint32_t etlsf_alloc_size  (etlsf_t arena, etlsf_alloc_t id);
uint32_t etlsf_alloc_offset(etlsf_t arena, etlsf_alloc_t id);

int etlsf_alloc_is_valid(etlsf_t arena, etlsf_alloc_t id);

#ifdef __cplusplus
}
#endif
