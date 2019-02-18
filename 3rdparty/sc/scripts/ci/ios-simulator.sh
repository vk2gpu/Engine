#!/bin/bash -ex

IPHONEOS_SDKS=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs
IPHONEOS_SDK_FOLDER="$(find "${IPHONEOS_SDKS}" -maxdepth 1 -type l | head -n 1)"

CMAKE_FLAGS="-DIOS_SIMULATOR=1 -DCMAKE_TOOLCHAIN_FILE=scripts/cmake/ios.cmake"
EXAMPLE_BUILD_ARGS="-arch i386 -arch x86_64 -isysroot ${IPHONEOS_SDK_FOLDER}"

source "$(dirname "$0")/helpers.sh"
osx_build

