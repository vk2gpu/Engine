#!/bin/bash -ex

source "$(dirname "$0")/helpers.sh"
dockcross_build

# Run unit tests
docker run --rm -e "WINEDEBUG=-all" -v "$PWD:/work" rhoot/wine32 wine /work/build-debug/bin/sc_tests --reporter console
docker run --rm -e "WINEDEBUG=-all" -v "$PWD:/work" rhoot/wine32 wine /work/build-release/bin/sc_tests --reporter console

# Run examples
for file in $(ls examples/*.c); do
    docker run --rm -e "WINEDEBUG=-all" -v "$PWD:/work" rhoot/wine32 wine "/work/${file%.*}" > /dev/null
done
