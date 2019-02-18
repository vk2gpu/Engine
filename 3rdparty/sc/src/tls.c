/*
 * Copyright (c) 2018 Johan Sköld
 * License: https://opensource.org/licenses/ISC
 */

#include "sc_p.h"

#if defined(_MSC_VER)
#   define THREAD_LOCAL __declspec(thread)
#else
#   define THREAD_LOCAL __thread
#endif

static THREAD_LOCAL context_data t_main;
static THREAD_LOCAL context_data* t_current;

NO_INLINE context_data* sc_get_main_context_data (void) {
    return &t_main;
}

NO_INLINE context_data* sc_get_curr_context_data (void) {
    return t_current;
}

NO_INLINE void sc_set_curr_context_data (context_data* data) {
    t_current = data;
}
