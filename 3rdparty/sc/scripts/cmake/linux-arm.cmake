# ---
#  Copyright (c) 2016 Johan Sköld
#  License: https://opensource.org/licenses/ISC
# ---

# Toolchain file to go with this docker container:
# https://hub.docker.com/r/rhoot/rpxc-cmake/
#
# Based on this docker container (see for docs):
# https://hub.docker.com/r/sdthirlwall/raspberry-pi-cross-compiler/

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_SYSROOT /rpxc/sysroot)

set(CMAKE_ASM_COMPILER /rpxc/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_C_COMPILER /rpxc/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER /rpxc/bin/arm-linux-gnueabihf-g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
