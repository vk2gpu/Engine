#!/bin/bash -ex

# Build debug
mkdir build-debug
cmake -DCMAKE_BUILD_TYPE=Debug -Bbuild-debug -H.
make -C build-debug
build-debug/bin/sc_tests --reporter console

# Build release
mkdir build-release
cmake -DCMAKE_BUILD_TYPE=Release -Bbuild-release -H.
make -C build-release
build-release/bin/sc_tests --reporter console

# Build and run examples
cd examples

for file in $(ls *.c); do
    cc -Wall -Werror -I../include -L../build-release/lib $file -lsc -o ${file%.*}
    ./${file%.*} > /dev/null
done
