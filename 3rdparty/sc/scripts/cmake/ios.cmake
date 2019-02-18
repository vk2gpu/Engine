# ---
#  Copyright (c) 2016 Johan Sköld
#  License: https://opensource.org/licenses/ISC
# ---

set(IOS ON)

#
# Developer base path
#

if(NOT DEFINED IOS_DEVELOPER_PATH)
    set(IOS_DEVELOPER_PATH "/Applications/Xcode.app/Contents/Developer")
endif()

if(NOT EXISTS "${IOS_DEVELOPER_PATH}")
    message(FATAL_ERROR "Could not find '${IOS_DEVELOPER_PATH}'. Please set IOS_DEVELOPER_PATH to a valid path.")
endif()

#
# iOS SDK options
#

if(NOT IOS_SIMULATOR)
    if(NOT DEFINED IOS_ARCHITECTURES)
        set(IOS_ARCHITECTURES armv7 armv7s arm64)
    endif()

    set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos")
    set(IOS_PLATFORM_TYPE "iPhoneOS")
else()
    if(NOT DEFINED IOS_ARCHITECTURES)
        set(IOS_ARCHITECTURES i386 x86_64)
    endif()

    set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphonesimulator")
    set(IOS_PLATFORM_TYPE "iPhoneSimulator")
endif()

# For some magical reason, we cannot use the SDK folder named simply iPhoneOS.sdk. We must use the
# numbered one...
#
# This command line fails to compile:
# /usr/bin/g++ -I/Users/johan/Git/sc/3rdparty/Catch/include -I/Users/johan/Git/sc/include -arch armv7 -arch armv7s -arch arm64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk -Wall -Werror -std=c++0x -o CMakeFiles/sc_tests.dir/tests/main.cpp.o -c /Users/johan/Git/sc/tests/main.cpp
#
# This command line does not:
# /usr/bin/g++ -I/Users/johan/Git/sc/3rdparty/Catch/include -I/Users/johan/Git/sc/include -arch armv7 -arch armv7s -arch arm64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS9.3.sdk -Wall -Werror -std=c++0x -o CMakeFiles/sc_tests.dir/tests/main.cpp.o -c /Users/johan/Git/sc/tests/main.cpp
#
# But:
# $ ls -l /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs
# total 8
# drwxrwxr-x  8 root  wheel  272 Mar  4 15:17 iPhoneOS.sdk
# lrwxr-xr-x  1 root  wheel   12 Jun 24 02:38 iPhoneOS9.3.sdk -> iPhoneOS.sdk
#
# ... <insert mind blown gif>

# Get all the folders within the iOS SDKs folder
file(GLOB IOS_SDK_PATH_NAMES
    "${IOS_DEVELOPER_PATH}/Platforms/${IOS_PLATFORM_TYPE}.platform/Developer/SDKs/${IOS_PLATFORM_TYPE}*.sdk")

# Find one that is a symlink
foreach(IOS_SDK_PATH_NAME ${IOS_SDK_PATH_NAMES})
    if (IS_SYMLINK "${IOS_SDK_PATH_NAME}")
        set(CMAKE_IOS_SDK_PATH "${IOS_SDK_PATH_NAME}" CACHE STRING "iOS SDK Path")
        break()
    endif()
endforeach()

# Universal binary support
set(CMAKE_OSX_ARCHITECTURES "${IOS_ARCHITECTURES}" CACHE STRING  "Build architecture for iOS")

# Compiler paths
set(CMAKE_ASM_COMPILER /usr/bin/clang)
set(CMAKE_C_COMPILER   /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

# Skip the platform compiler checks
set(CMAKE_ASM_COMPILER_WORKS TRUE)
set(CMAKE_C_COMPILER_WORKS   TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

# iOS SDK
set(CMAKE_FIND_ROOT_PATH "${CMAKE_IOS_SDK_PATH}")
set(CMAKE_OSX_SYSROOT    "${CMAKE_IOS_SDK_PATH}")

# Only look for libraries / includes within the sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
