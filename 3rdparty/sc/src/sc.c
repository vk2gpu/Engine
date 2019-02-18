/*
 * Copyright (c) 2016 Johan Sköld
 * License: https://opensource.org/licenses/ISC
 */

#include <sc.h>

#include <assert.h>     /* assert */
#include <stdint.h>     /* uintptr_t */

#include "sc_p.h"

/*
 * Compatibility
 */

#if defined(_MSC_VER)
#   define ALIGNOF(x)   __alignof(x)
#else
#   define ALIGNOF(x)   __alignof__(x)
#endif

/*
 * Private implementation
 */

static uintptr_t align_down (uintptr_t addr, uintptr_t alignment) {
    assert(alignment > 0);
    assert((alignment & (alignment - 1)) == 0);
    return addr & ~(alignment - 1);
}

static void context_proc (sc_transfer_t transfer) {
    context_data* data = (context_data*)transfer.data;
    assert(data != NULL);

    /* Jump back to parent */
    transfer = sc_jump_context(transfer.ctx, NULL);

    /* Update the current context */
    sc_current_context()->ctx = transfer.ctx;
    sc_set_curr_context_data(data);
    data->ctx = NULL;

    /* Execute the context proc */
    data->proc(transfer.data);
}

/*
 * Public implementation
 */

sc_context_t SC_CALL_DECL sc_context_create (
    void* stack_ptr,
    size_t stack_size,
    sc_context_proc_t proc
) {
    uintptr_t stack_addr;
    uintptr_t sp_addr;
    uintptr_t data_addr;
    sc_context_sp_t ctx;
    context_data* data;

    assert(stack_ptr != NULL);
    assert(stack_size >= SC_MIN_STACK_SIZE);
    assert(proc != NULL);

    /* Determine the bottom of the stack */
    stack_addr = (uintptr_t)stack_ptr;
    sp_addr = stack_addr + stack_size;

    /* Reserve some space at the bottom for the context data */
    data_addr = sp_addr - sizeof(context_data);
    data_addr = align_down(data_addr, ALIGNOF(context_data));
    assert(data_addr > stack_addr);
    sp_addr = data_addr;

    /* Align the stack pointer to a 64-byte boundary */
    sp_addr = align_down(sp_addr, 64);
    assert(sp_addr > stack_addr);

    /* Determine the new stack size */
    stack_size = sp_addr - stack_addr;

    /* Create the context */
    ctx = sc_make_context((void*)sp_addr, stack_size, context_proc);
    assert(ctx != NULL);

    /* Create the context data at the reserved address */
    data = (context_data*)data_addr;
    data->proc = proc;
    data->parent = sc_current_context();
    data->user_data = NULL;

    /* Transfer the proc pointer to the context by briefly switching to it */
    data->ctx = sc_jump_context(ctx, data).ctx;
    return data;
}

void SC_CALL_DECL sc_context_destroy (sc_context_t context) {
    assert(context != sc_current_context());
    assert(context != sc_main_context());

    sc_free_context(context->ctx);
}

void* SC_CALL_DECL sc_switch (sc_context_t target, void* value) {
    context_data* this_ctx = sc_current_context();
    sc_transfer_t transfer;

    assert(target != NULL);

    if (target != this_ctx) {
        transfer = sc_jump_context(target->ctx, value);
        sc_current_context()->ctx = transfer.ctx;
        sc_set_curr_context_data(this_ctx);
        this_ctx->ctx = NULL;
        value = transfer.data;
    }

    return value;
}

void* SC_CALL_DECL sc_yield (void* value) {
    context_data* current = sc_current_context();
    assert(current->parent != NULL);
    return sc_switch(current->parent, value);
}

void SC_CALL_DECL sc_set_data (sc_context_t context, void* data) {
    context->user_data = data;
}

void* SC_CALL_DECL sc_get_data (sc_context_t context) {
    return context->user_data;
}

sc_state_t SC_CALL_DECL sc_get_state (sc_context_t context) {
    sc_state_t state;
    sc_context_state(&state, context->ctx);
    return state;
}

sc_context_t SC_CALL_DECL sc_current_context (void) {
    context_data* current = sc_get_curr_context_data();
    return current ? current : sc_get_main_context_data();
}

sc_context_t SC_CALL_DECL sc_parent_context (void) {
    return sc_current_context()->parent;
}

sc_context_t SC_CALL_DECL sc_main_context (void) {
    return sc_get_main_context_data();
}
