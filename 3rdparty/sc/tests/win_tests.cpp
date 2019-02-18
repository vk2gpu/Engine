//---
// Copyright (c) 2016 Johan Sköld
// License: https://opensource.org/licenses/ISC
//---

#if defined(_WIN32)

#include "framework.hpp"
#include <sc.h>
#include <cstdint>

//
// Win64 ABI tests
//

#if defined(_WIN64)

#include <emmintrin.h>

extern "C" __m128i get_xmm_register (int index);
extern "C" void set_xmm_register (int index, __m128i value);

namespace {

    void init_xmm_registers(int base) {
        auto index = base;

        for (auto reg = 6; reg <= 15; ++reg) {
            const auto value = _mm_set_epi32(index, index + 1, index + 2, index + 3);
            set_xmm_register(reg, value);
            index += 4;
        }
    }

    bool verify_xmm_registers(int base) {
        int index = base;

        for (auto reg = 6; reg <= 15; ++reg) {
            const auto expected = _mm_set_epi32(index, index + 1, index + 2, index + 3);
            const auto value = get_xmm_register(reg);
            const auto equals = _mm_cmpeq_epi32(value, expected);
            const auto mask = _mm_movemask_epi8(equals);

            if (mask != 0xffff) {
                return false;
            }

            index += 4;
        }

        return true;
    }

}

//
// Test procs
//

extern "C" void get_rsp_proc (void* rsp);

namespace {

    void SC_CALL_DECL test_xmm_registers_proc (void* base) {
        auto baseIndex = *(int*)base;
        init_xmm_registers(baseIndex);
        sc_yield(nullptr);
        auto success = verify_xmm_registers(baseIndex);
        sc_yield(&success);
    }

}

//
// Tests
//

DESCRIBE("Windows x64 ABI") {

    IT("should have a 16-byte aligned stack on context entry") {
        uintptr_t rsp = 0;

        uint8_t stack[SC_MIN_STACK_SIZE];
        auto context = sc_context_create(stack, sizeof(stack), get_rsp_proc);
        sc_switch(context, &rsp);
        sc_context_destroy(context);

        // While the alignment should be at 16 bytes, that is before the
        // return address is pushed onto the stack. As such, when the proc is
        // actually entered, rsp should end in 8.
        REQUIRE((rsp & 15) == 8);
    }

    IT("should preserve [xmm6..xmm15] on context switch") {
        const int mainBase = 0x1000;
        const int ctxBase = 0x2000;

        uint8_t stack[SC_MIN_STACK_SIZE];
        auto context = sc_context_create(stack, sizeof(stack), test_xmm_registers_proc);

        // Set all non-volatile XMM registers for the main context
        init_xmm_registers(mainBase);

        // Allow the context to set *its* non-volatile XMM registers
        sc_switch(context, (int*)&ctxBase);

        // Verify that the registers are still what we originally set them to
        auto verified = verify_xmm_registers(mainBase);
        REQUIRE(verified == true);

        // Allow the context to verify *its* registers
        verified = *(bool*)sc_switch(context, NULL);
        REQUIRE(verified == true);

        sc_context_destroy(context);
    }

}

#endif // defined(_WIN64)

//
// Win32 ABI tests
//

#if !defined(_WIN64)

namespace {

    void SC_CALL_DECL test_esp_eip_null(void*) {
        uint32_t ebpReg;
        uint32_t eipReg;

#if defined(_MSC_VER)
        __asm {
        eiphelper:
            lea ecx, eiphelper
            mov ebpReg, ebp
            mov eipReg, ecx
        }
#else
        asm (
            R"(
            eiphelper:
                movl %%ebp, %0
                leal eiphelper, %1
            )"
            : "=r"(ebpReg), "=r"(eipReg)
        );
#endif

        while (ebpReg) {
            eipReg = *(uint32_t*)(ebpReg + 4);
            ebpReg = *(uint32_t*)(ebpReg);
        }

        bool success = !ebpReg && !eipReg;
        sc_yield(&success);
    }

}

DESCRIBE("Windows x86 ABI") {

    IT("should have a NULL stack base and return pointer at the bottom of the stack") {
        uint8_t stack[SC_MIN_STACK_SIZE];
        auto context = sc_context_create(stack, sizeof(stack), test_esp_eip_null);
        REQUIRE(context != nullptr);
        auto success = (bool*)sc_switch(context, nullptr);
        REQUIRE(success);
        sc_context_destroy(context);
    }

}

#endif

//
// Win32 API tests
//
// Cannot be built in MinGW since it may be cross compiling on another OS.
//

#if defined(_MSC_VER)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace {

    void SC_CALL_DECL test_fiber_data(void*) {
        sc_yield(GetCurrentFiber());
    }

}

DESCRIBE("Win32 API") {

    IT("should return NULL for GetCurrentFiber()") {
        uint8_t stack[SC_MIN_STACK_SIZE];
        auto context = sc_context_create(stack, sizeof(stack), test_fiber_data);
        REQUIRE(context != nullptr);
        REQUIRE(sc_switch(context, nullptr) == nullptr);
        sc_context_destroy(context);
    }

}

#endif // defined(_MSC_VER)

#endif // defined(_WIN32)
