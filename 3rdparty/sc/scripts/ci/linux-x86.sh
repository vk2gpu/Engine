#!/bin/bash -ex

source "$(dirname "$0")/helpers.sh"
dockcross_build

# Run unit tests. For some reason travis has troubles running these directly,
# so run them in the container.
./dockcross /work/build-debug/bin/sc_tests --reporter console
./dockcross /work/build-release/bin/sc_tests --reporter console

# Run examples.
for file in $(ls examples/*.c); do
    ./dockcross /work/${file%.*} > /dev/null
done
