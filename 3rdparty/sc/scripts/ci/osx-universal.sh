#!/bin/bash -ex

CMAKE_FLAGS="-DCMAKE_OSX_ARCHITECTURES=i386;x86_64"
EXAMPLE_BUILD_ARGS="-arch i386 -arch x86_64"

source "$(dirname "$0")/helpers.sh"
osx_build

# Run unit tests
build-debug/bin/sc_tests --reporter console
build-release/bin/sc_tests --reporter console

# Run examples
for file in $(ls examples/*.c); do
    ${file%.*} > /dev/null
done
