#pragma once

//////////////////////////////////////////////////////////////////////////
// Platform Identification
#define PLATFORM_HTML5 0
#define PLATFORM_ANDROID 0
#define PLATFORM_LINUX 0
#define PLATFORM_WINDOWS 0
#define PLATFORM_WINPHONE 0
#define PLATFORM_IOS 0
#define PLATFORM_OSX 0

// Emscripten (HTML5)
#if defined(EMSCRIPTEN) || defined(__EMSCRIPTEN__)
#undef PLATFORM_HTML5
#define PLATFORM_HTML5 1

// Android
#elif defined(__ANDROID__)
#undef PLATFORM_ANDROID
#define PLATFORM_ANDROID 1

// Linux
#elif defined(linux) || defined(__linux)
#undef PLATFORM_LINUX
#define PLATFORM_LINUX 1

// Windows
#elif defined(WINDOWS) || defined(_WINDOWS)
#undef PLATFORM_WINDOWS
#define PLATFORM_WINDOWS 1

// Windows
#elif defined(WINPHONE) || defined(_WINPHONE)
#undef PLATFORM_WINPHONE
#define PLATFORM_WINPHONE 1

// iOS
#elif defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
#undef PLATFORM_IOS
#define PLATFORM_IOS 1

// Mac OSX
#elif defined(__APPLE__) && defined(__MACH__)
#undef PLATFORM_OSX
#define PLATFORM_OSX 1

// Error.
#else
#error "Unknown platform!"

// END.
#endif

//////////////////////////////////////////////////////////////////////////
// Architecture Identification

// X86_64
#if defined(__ia64__) || defined(__IA64__) || defined(_IA64) || defined(_M_IA64) || defined(__x86_64__) ||             \
    defined(_M_X64)
#define ARCH_X86_64 1
#define ENDIAN_LITTLE 1
#define ENDIAN_BIG 0
#define CACHE_LINE_SIZE 64
#define PLATFORM_ALIGNMENT 16

// i386 (and higher)
#elif defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(_M_IX86)
#define ARCH_X86 1
#define ENDIAN_LITTLE 1
#define ENDIAN_BIG 0
#define CACHE_LINE_SIZE 64
#define PLATFORM_ALIGNMENT 16

// ARM
#elif defined(__arm__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7S__) || defined(TARGET_OS_IPHONE) ||         \
    defined(_M_ARM)
#define ARCH_ARM 1
#define ENDIAN_LITTLE 1
#define ENDIAN_BIG 0
#define CACHE_LINE_SIZE 64
#define PLATFORM_ALIGNMENT 16

// THUMB
#elif defined(__thumb__)
#define ARCH_THUMB 1
#define ENDIAN_LITTLE 1
#define ENDIAN_LITTLE 0
#define CACHE_LINE_SIZE 64
#define PLATFORM_ALIGNMENT 4

// Emscripten (asm.js)
#elif defined(EMSCRIPTEN)
#define ARCH_ASMJS 1
#define ENDIAN_LITTLE 1
#define ENDIAN_BIG 0
#define CACHE_LINE_SIZE 64
#define PLATFORM_ALIGNMENT 4

// END.
#endif

#if !defined(ENDIAN_LITTLE) || !defined(ENDIAN_BIG)
#error "No endianess defined."
#endif

//////////////////////////////////////////////////////////////////////////
// Compiler Identification

// GCC
#if defined(__GNU__) || defined(__GNUG__)
#define COMPILER_GCC 1
#define NOEXCEPT noexcept
#define FORCEINLINE
#else
#define COMPILER_GCC 0
#endif

// Clang
#if defined(__llvm__)
#define COMPILER_CLANG 1
#define NOEXCEPT noexcept
#define FORCEINLINE
#else
#define COMPILER_CLANG 0
#endif

// Codewarrior
#if defined(__MWERKS__)
#define COMPILER_CW 1
#define NOEXCEPT
#define FORCEINLINE
#else
#define COMPILER_CW 0
#endif

// MSVC
#if defined(_MSC_VER)
#define COMPILER_MSVC 1
#define NOEXCEPT
#define FORCEINLINE __forceinline
#else
#define COMPILER_MSVC 0
#endif


//////////////////////////////////////////////////////////////////////////
// Inlining.
#ifdef _DEBUG
#define CODE_INLINE 0
#define INLINE
#else
#define CODE_INLINE 1
#define INLINE inline
#endif

//////////////////////////////////////////////////////////////////////////
// Deprecated markers.
#if COMPILER_MSVC
#define DEPRECATED(_m) __declspec(deprecated)
#else
#define DEPRECATED(_m) [[deprecated(_m)]]
#endif

//////////////////////////////////////////////////////////////////////////
// Build type.
#ifdef _DEBUG
#define DEBUG
#endif

#ifdef NDEBUG
#define RELEASE
#endif

//////////////////////////////////////////////////////////////////////////
// Disable warnings.
#if COMPILER_MSVC
/// nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
/// '<member_type>' needs to have dll-interface to be used by clients of class '<containing_type>'
#pragma warning(disable : 4251)
/// conditional expression is constant
#pragma warning(disable : 4127)
#endif

//////////////////////////////////////////////////////////////////////////
// Symbol exporting.
#if COMPILER_MSVC
#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)
#else
#define EXPORT
#define IMPORT
#endif
