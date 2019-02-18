//---
// Copyright (c) 2016 Johan Sköld
// License: https://opensource.org/licenses/ISC
//---

#include "framework.hpp"

#include <cstdint> // uint8_t

#include <sc.h>

//
// Test procs
//

namespace {

    void SC_CALL_DECL empty_proc (void*) {}

    void SC_CALL_DECL set_to_true_proc (void* param) {
        *(bool*)param = true;
        sc_yield(nullptr);
    }

    void SC_CALL_DECL yield_current_proc (void*) {
        sc_yield(sc_current_context());
    }

    void SC_CALL_DECL yield_main_proc (void*) {
        sc_yield(sc_main_context());
    }

    void SC_CALL_DECL yield_parent_proc (void*) {
        sc_yield(sc_parent_context());
    }

    void SC_CALL_DECL recursive_proc (void*) {
        uint8_t stack[SC_MIN_STACK_SIZE];
        auto context = sc_context_create(stack, sizeof(stack), yield_main_proc);
        auto yielded = sc_switch(context, NULL);
        sc_context_destroy(context);
        sc_yield(yielded);
    }

} // namespace

//
// sc_context_create tests
//

DESCRIBE("sc_context_create") {

    IT("should return a valid context") {
        bool result = false;

        uint8_t stack[SC_MIN_STACK_SIZE];
        auto context = sc_context_create(stack, sizeof(stack), set_to_true_proc);
        sc_switch(context, &result);
        sc_context_destroy(context);

        REQUIRE(result == true);
    }

}

//
// sc_context_destroy tests
//

DESCRIBE("sc_context_destroy") {

    IT("should not crash") {
        uint8_t stack[SC_MIN_STACK_SIZE];
        auto context = sc_context_create(stack, sizeof(stack), empty_proc);
        sc_context_destroy(context);
    }

}

//
// sc_switch tests
//

DESCRIBE("sc_switch") {

    GIVEN("a valid context") {
        IT("should switch to that context") {
            auto executed = false;

            uint8_t stack[SC_MIN_STACK_SIZE];
            auto context = sc_context_create(stack, sizeof(stack), set_to_true_proc);
            sc_switch(context, &executed);
            sc_context_destroy(context);

            REQUIRE(executed == true);
        }
    }

    GIVEN("the current context") {
        IT("should return the passed-in value") {
            int marker;
            auto result = sc_switch(sc_current_context(), &marker);
            REQUIRE(result == &marker);
        }
    }

}

//
// sc_yield tests
//

DESCRIBE("sc_yield") {

    IT("should switch to the parent context") {
        uint8_t stack[8192];
        auto context = sc_context_create(stack, sizeof(stack), recursive_proc);
        auto yielded = sc_switch(context, NULL);
        sc_context_destroy(context);

        REQUIRE(yielded == sc_main_context());
    }

}

//
// sc_set_data
//

DESCRIBE("sc_set_data") {

    IT("should store the pointer") {
        uint8_t stack[SC_MIN_STACK_SIZE];
        auto context = sc_context_create(stack, sizeof(stack), empty_proc);
        auto pointer = (void*)uintptr_t(0xbadf00d);
        sc_set_data(context, pointer);
        REQUIRE(sc_get_data(context) == pointer);
        sc_context_destroy(context);
    }

}

//
// sc_get_data
//

DESCRIBE("sc_get_data") {

    IT("should default to NULL") {
        uint8_t stack[SC_MIN_STACK_SIZE];
        auto context = sc_context_create(stack, sizeof(stack), empty_proc);
        REQUIRE(sc_get_data(context) == nullptr);
        sc_context_destroy(context);
    }

    IT("should get the pointer") {
        uint8_t stack[SC_MIN_STACK_SIZE];
        auto context = sc_context_create(stack, sizeof(stack), empty_proc);
        auto pointer = (void*)uintptr_t(0xbadf00d);
        sc_set_data(context, pointer);
        REQUIRE(sc_get_data(context) == pointer);
        sc_context_destroy(context);
    }

}

//
// sc_current_context tests
//

DESCRIBE("sc_current_context") {

    IT("should return the main context when not context switched") {
        REQUIRE(sc_current_context() == sc_main_context());
    }

    IT("should return the sc_context_t of the currently executing context") {
        uint8_t stack[SC_MIN_STACK_SIZE];
        auto context = sc_context_create(stack, sizeof(stack), yield_current_proc);
        auto current = sc_switch(context, nullptr);
        sc_context_destroy(context);

        REQUIRE(context == current);
    }

}

//
// sc_parent_context tests
//

DESCRIBE("sc_parent_context") {

    IT("should return the parent context") {
        uint8_t stack[SC_MIN_STACK_SIZE];
        auto context = sc_context_create(stack, sizeof(stack), yield_parent_proc);
        auto parent = sc_switch(context, nullptr);
        sc_context_destroy(context);

        REQUIRE(sc_main_context() == parent);
    }

}

//
// sc_main_context tests
//

DESCRIBE("sc_main_context") {

    IT("should always return the main context") {
        uint8_t stack[SC_MIN_STACK_SIZE];
        auto context = sc_context_create(stack, sizeof(stack), yield_main_proc);
        auto main = sc_switch(context, nullptr);
        sc_context_destroy(context);

        REQUIRE(sc_main_context() == main);
    }

}

//
// sc_get_state tests
//

#if defined(_M_IX86) || defined(_X86_) || defined(__i386__)
#   define SC_HAS_GET_STATE_IMPL
#elif defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__amd64__)
#   define SC_HAS_GET_STATE_IMPL
#elif defined(__arm__) || defined(__arm64__)
#   define SC_HAS_GET_STATE_IMPL
#endif

DESCRIBE("sc_get_state") {

#if defined(SC_HAS_GET_STATE_IMPL)

    IT("should return the current context's state correctly") {
        auto state = sc_get_state(sc_current_context());

        // We can't really test anything useful besides the CPU type, since we
        // have no idea what the registers should be set to.
        const auto isX86 = state.type == SC_CPU_TYPE_X86;
        const auto isX64 = state.type == SC_CPU_TYPE_X64;
        const auto isArm = state.type == SC_CPU_TYPE_ARM;
        REQUIRE((isX86 || isX64 || isArm));
    }

    IT("should return a yielded context's state") {
        uint8_t stack[SC_MIN_STACK_SIZE];
        const auto stackEnd = stack + sizeof(stack);

        auto context = sc_context_create(stack, sizeof(stack), yield_main_proc);
        sc_switch(context, nullptr);
        auto state = sc_get_state(context);
        sc_context_destroy(context);

        switch (state.type) {
            case SC_CPU_TYPE_ARM:
                REQUIRE(state.registers.arm.sp >= (uintptr_t)stack);
                REQUIRE(state.registers.arm.sp <= (uintptr_t)stackEnd);
                REQUIRE(state.registers.arm.pc >= (uintptr_t)&sc_switch);
                REQUIRE(state.registers.arm.pc <= (uintptr_t)&sc_switch + 0x1000);
                break;

            case SC_CPU_TYPE_X86:
                REQUIRE(state.registers.x86.esp >= (uintptr_t)stack);
                REQUIRE(state.registers.x86.esp <= (uintptr_t)stackEnd);

                // We can't really test any other registers, since we don't
                // know what values they should have. As for `eip`, we know it
                // should be inside sc_switch, but thanks to MSVC's jump tables
                // we can't determine an address range for that function.
                break;

            case SC_CPU_TYPE_X64:
                REQUIRE(state.registers.x64.rsp >= (uintptr_t)stack);
                REQUIRE(state.registers.x64.rsp <= (uintptr_t)stackEnd);

                // We can't test any other registers, same as for x86.
                break;

            default:
                REQUIRE(!"Invalid return value.");
        }
    }

#else

    IT("should result in SC_CPU_TYPE_UNKNOWN") {
        auto state = sc_get_state(sc_current_context());
        REQUIRE(state.type == SC_CPU_TYPE_UNKNOWN);
    }

#endif

}
