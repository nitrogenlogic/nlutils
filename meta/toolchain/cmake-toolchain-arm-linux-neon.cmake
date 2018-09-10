# CMake toolchain file for OMAP-3 and related Cortex-A8 processors with Neon
# Copyright (C)2011 Mike Bourgeous.  Released under AGPLv3 in 2018.

# See http://www.cmake.org/Wiki/CMake_Cross_Compiling
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)
set(CMAKE_C_COMPILER arm-linux-gnueabi-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabi-g++)
set(CMAKE_FIND_ROOT_PATH $ENV{DEBIAN_ROOT} $ENV{LIBS_ROOT} $ENV{ROOT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_C_FLAGS "-march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp -fsingle-precision-constant -I$ENV{DEBIAN_ROOT}/usr/include -I$ENV{DEBIAN_ROOT}/usr/include/libusb-1.0 -I$ENV{LIBS_ROOT}/usr/include -I$ENV{LIBS_ROOT}/usr/include/libusb-1.0 -I$ENV{LIBS_ROOT}/usr/local/include -I$ENV{ROOT}/usr/include -I$ENV{ROOT}/usr/local/include" CACHE STRING "C compiler flags" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O2 -ftree-vectorize")
set(CMAKE_C_FLAGS_MINSIZEREL "-O2 -ftree-vectorize")

add_definitions(-DNL_EMBEDDED)
set(NL_EMBEDDED true)

# TODO: Create CMake find scripts instead of using hard-coded include and library paths
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-rpath $ENV{LIBS_ROOT}/usr/lib -Wl,-rpath $ENV{DEBIAN_ROOT}/usr/lib -Wl,-rpath $ENV{ROOT}/usr/local/lib -L$ENV{LIBS_ROOT}/usr/lib -L $ENV{DEBIAN_ROOT}/usr/lib -L$ENV{ROOT}/usr/local/lib" CACHE STRING "Linker flags" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-rpath $ENV{LIBS_ROOT}/usr/lib -Wl,-rpath $ENV{DEBIAN_ROOT}/usr/lib -Wl,-rpath $ENV{ROOT}/usr/local/lib -L$ENV{LIBS_ROOT}/usr/lib -L $ENV{DEBIAN_ROOT}/usr/lib -L$ENV{ROOT}/usr/local/lib" CACHE STRING "Linker flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "-Wl,-rpath $ENV{LIBS_ROOT}/usr/lib -Wl,-rpath $ENV{DEBIAN_ROOT}/usr/lib -Wl,-rpath $ENV{ROOT}/usr/local/lib -L$ENV{LIBS_ROOT}/usr/lib -L $ENV{DEBIAN_ROOT}/usr/lib -L$ENV{ROOT}/usr/local/lib" CACHE STRING "Linker flags" FORCE)
