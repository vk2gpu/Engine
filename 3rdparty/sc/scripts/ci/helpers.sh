dockcross_build_example() {
    ./dockcross bash -c "\$CC -Wall -Werror -I/work/include -L/work/build-release/lib $1.c -lsc -o $1"
}

dockcross_build() {
    # Extract the compiler helper script from the docker container
    docker run --rm "dockcross/$target" > dockcross
    chmod +x dockcross

    ./dockcross cc --version
    ./dockcross c++ --version
    ./dockcross cmake --version

    # Generate build files, and build
    mkdir build-debug
    mkdir build-release
    ./dockcross cmake $CMAKE_FLAGS -DCMAKE_BUILD_TYPE=Debug -B/work/build-debug -H/work
    ./dockcross cmake $CMAKE_FLAGS -DCMAKE_BUILD_TYPE=Release -B/work/build-release -H/work
    ./dockcross make -C /work/build-debug VERBOSE=1
    ./dockcross make -C /work/build-release VERBOSE=1

    # Build examples
    for file in $(ls examples/*.c); do
        example_name=${file%.*}
        dockcross_build_example "$example_name"
        file "$example_name"
    done
}

osx_build_example() {
    local name="$1"
    shift 1
    cc -Wall -Werror $EXAMPLE_BUILD_ARGS -Iinclude -Lbuild-release/lib "$name.c" -lsc -o "$name"
}

osx_build() {
    # Update cmake
    brew update
    if brew outdated --quiet | grep -qx cmake; then brew upgrade cmake; fi

    # Generate build files, and build
    mkdir build-debug
    mkdir build-release
    cmake $CMAKE_FLAGS -DCMAKE_BUILD_TYPE=Debug -Bbuild-debug -H.
    cmake $CMAKE_FLAGS -DCMAKE_BUILD_TYPE=Release -Bbuild-release -H.
    make -C build-debug VERBOSE=1
    make -C build-release VERBOSE=1

    # Build examples
    for file in $(ls examples/*.c); do
        example_name=${file%.*}
        osx_build_example "$example_name"
        file "$example_name"
    done
}

