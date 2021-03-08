# CMake toolchain file for Sheeva and related ARMv5 processors with no FPU
# Copyright (C)2011-2020 Mike Bourgeous.  Released under AGPLv3 in 2018.

# This is pretty hacked together to use a local cross-compiler with headers and
# libraries from an armel chroot.  It might be better to use Docker to build a
# native container out of the target Debian version and use its standard
# cross-compiling setup.

# See http://www.cmake.org/Wiki/CMake_Cross_Compiling
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv5tel)
set(CMAKE_C_COMPILER arm-linux-gnueabi-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabi-g++)
set(CMAKE_FIND_ROOT_PATH $ENV{DEBIAN_ROOT} $ENV{LIBS_ROOT} $ENV{ROOT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_C_FLAGS "-g -march=armv5te -mtune=xscale -fsingle-precision-constant -nostdinc -I$ENV{DEBIAN_ROOT}/usr/include -I$ENV{DEBIAN_ROOT}/usr/include/arm-linux-gnueabi -I$ENV{DEBIAN_ROOT}/usr/lib/gcc/arm-linux-gnueabi/6/include -I$ENV{DEBIAN_ROOT}/usr/lib/gcc/arm-linux-gnueabi/6/include-fixed -I$ENV{DEBIAN_ROOT}/usr/include/libusb-1.0 -I$ENV{LIBS_ROOT}/usr/include -I$ENV{LIBS_ROOT}/usr/include/arm-linux-gnueabi -I$ENV{LIBS_ROOT}/usr/lib/gcc/arm-linux-gnueabi/6/include/ -I$ENV{LIBS_ROOT}/usr/include/libusb-1.0 -I$ENV{LIBS_ROOT}/usr/local/include -I$ENV{ROOT}/usr/include -I$ENV{ROOT}/usr/include/arm-linux-gnueabi -I$ENV{ROOT}/usr/lib/gcc/arm-linux-gnueabi/6/include/ -I$ENV{ROOT}/usr/local/include" CACHE STRING "C compiler flags" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_C_FLAGS_MINSIZEREL "-O2")

add_definitions(-DNL_EMBEDDED)
set(NL_EMBEDDED true)

# TODO: Create CMake find scripts instead of using hard-coded include and
# library paths, or just use a Docker image of the target Debian version
set(NL_LINKER_FLAGS "-Wl,-nostdlib -nodefaultlibs -nostartfiles $ENV{DEBIAN_ROOT}/usr/lib/arm-linux-gnueabi/crt1.o $ENV{DEBIAN_ROOT}/usr/lib/arm-linux-gnueabi/crti.o -lgcc_s -lc $ENV{DEBIAN_ROOT}/usr/lib/arm-linux-gnueabi/libc.a -Wl,-rpath $ENV{LIBS_ROOT}/usr/lib/arm-linux-gnueabi -Wl,-rpath $ENV{DEBIAN_ROOT}/lib/arm-linux-gnueabi -Wl,-rpath $ENV{DEBIAN_ROOT}/usr/lib/arm-linux-gnueabi -Wl,-rpath $ENV{DEBIAN_ROOT}/usr/lib/gcc/arm-linux-gnueabi/6 -Wl,-rpath $ENV{ROOT}/usr/lib/arm-linux-gnueabi -L$ENV{LIBS_ROOT}/usr/local/lib -L$ENV{DEBIAN_ROOT}/usr/local/lib -L$ENV{ROOT}/usr/local/lib")
set(CMAKE_SHARED_LINKER_FLAGS ${NL_LINKER_FLAGS} CACHE STRING "Linker flags" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS ${NL_LINKER_FLAGS} CACHE STRING "Linker flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS ${NL_LINKER_FLAGS} CACHE STRING "Linker flags" FORCE)
