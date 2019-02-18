#!/bin/bash -ex

IPHONEOS_SDKS=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs
IPHONEOS_SDK_FOLDER="$(find "${IPHONEOS_SDKS}" -maxdepth 1 -type l | head -n 1)"

CMAKE_FLAGS="-DCMAKE_TOOLCHAIN_FILE=scripts/cmake/ios.cmake"
EXAMPLE_BUILD_ARGS="-arch armv7 -arch armv7s -arch arm64 -isysroot ${IPHONEOS_SDK_FOLDER}"

source "$(dirname "$0")/helpers.sh"
osx_build
