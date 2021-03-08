# CMake toolchain file for Sheeva and related ARMv5 processors with no FPU
# Copyright (C)2011 Mike Bourgeous.  Released under AGPLv3 in 2018.

# See http://www.cmake.org/Wiki/CMake_Cross_Compiling
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv5tel)
set(CMAKE_C_COMPILER arm-linux-gnueabi-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabi-g++)
set(CMAKE_FIND_ROOT_PATH $ENV{DEBIAN_ROOT} $ENV{LIBS_ROOT} $ENV{ROOT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_C_FLAGS "-march=armv5te -mtune=xscale -fsingle-precision-constant -I$ENV{DEBIAN_ROOT}/usr/include -I$ENV{DEBIAN_ROOT}/usr/include/libusb-1.0 -I$ENV{LIBS_ROOT}/usr/include -I$ENV{LIBS_ROOT}/usr/include/libusb-1.0 -I$ENV{LIBS_ROOT}/usr/local/include -I$ENV{ROOT}/usr/include -I$ENV{ROOT}/usr/local/include" CACHE STRING "C compiler flags" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_C_FLAGS_MINSIZEREL "-O2")

add_definitions(-DNL_EMBEDDED)
set(NL_EMBEDDED true)

# TODO: Create CMake find scripts instead of using hard-coded include and library paths
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-rpath $ENV{LIBS_ROOT}/usr/lib/arm-linux-gnueabi -Wl,-rpath $ENV{DEBIAN_ROOT}/usr/lib/arm-linux-gnueabi -Wl,-rpath $ENV{ROOT}/usr/lib/arm-linux-gnueabi -L$ENV{LIBS_ROOT}/usr/local/lib -L$ENV{DEBIAN_ROOT}/usr/local/lib -L$ENV{ROOT}/usr/local/lib" CACHE STRING "Linker flags" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-rpath $ENV{LIBS_ROOT}/usr/lib/arm-linux-gnueabi -Wl,-rpath $ENV{DEBIAN_ROOT}/usr/lib/arm-linux-gnueabi -Wl,-rpath $ENV{ROOT}/usr/lib/arm-linux-gnueabi -L$ENV{LIBS_ROOT}/usr/local/lib -L$ENV{DEBIAN_ROOT}/usr/local/lib -L$ENV{ROOT}/usr/local/lib" CACHE STRING "Linker flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS    "-Wl,-rpath $ENV{LIBS_ROOT}/usr/lib/arm-linux-gnueabi -Wl,-rpath $ENV{DEBIAN_ROOT}/usr/lib/arm-linux-gnueabi -Wl,-rpath $ENV{ROOT}/usr/lib/arm-linux-gnueabi -L$ENV{LIBS_ROOT}/usr/local/lib -L$ENV{DEBIAN_ROOT}/usr/local/lib -L$ENV{ROOT}/usr/local/lib" CACHE STRING "Linker flags" FORCE)
