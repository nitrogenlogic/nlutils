#!/bin/bash
# Cross-compilation script for nlutils.
# Copyright (C)2012-2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
#
# Installs base programs and libraries into a root directory, development
# programs, libraries, and headers into a libs directory.
#
# The first parameter is the architecture nickname ("neon" for armv7l with Neon
# and VFP (e.g. old BeagleBoard), "nofp" for armv5te (e.g. SheevaPlug), or
# "armhf" for armv7l with hard-float ABI (e.g. newer CuBox)).
#
# The second parameter is optional, and specifies a suffix to append to the
# default installation root (e.g. "depth").
#
# The ROOT environment variable controls where the base library and programs
# will be installed.
#
# Setting the ROOT environment variable or passing a suffix will disable
# library+header installation.  Setting the LIBS_ROOT and LIBS_PREFIX
# environment variables will re-enable library+header installation.
#
# Examples:
#
# Install runtime files into ~/devel/crosscompile/cross-root-arm-nofp,
# development files into ~/devel/crosscompile/cross-libs-arm-nofp:
#
#	./crosscompile.sh nofp
#
#
# Install runtime files into ~/devel/crosscompile/cross-root-arm-neon-depth:
#
#	./crosscompile.sh neon depth
#
# Use the BUILD environment variable to specify a Debug build:
#
#	BUILD=Debug ./crosscompile.sh
#
# Install armv5te runtime files into /tmp/nofp-root, development files into
# /tmp/nofp-libs:
#
#	ROOT=/tmp/nofp-root LIBS_ROOT=/tmp/nofp-libs ./crosscompile.sh nofp

set -e

CROSS_BASE=${HOME}/devel/crosscompile
DEBIAN_VERSION=${DEBIAN_VERSION:-buster}

BASEDIR="$(readlink -m "$(dirname "$0")")"
NCPUS=$(grep -i 'processor.*:' /proc/cpuinfo | wc -l)

if [ "$#" -eq 0 ]; then
	echo "Building all embedded targets.  Press Ctrl-C in 5 seconds to abort."
	sleep 5
	"$0" neon
	"$0" nofp
	# TODO: enable when automatic libs are available
	# "$0" armhf
	"$0" neon depth
	"$0" nofp depth
	# TODO: enable when automatic libs are available
	# "$0" armhf depth
	exit
fi

TARGET="$1"
case "$TARGET" in
	neon)
		TOOLCHAIN=${BASEDIR}/meta/toolchain/cmake-toolchain-arm-linux-neon.cmake
		DEFAULT_ROOT=${CROSS_BASE}/cross-root-arm-neon-${DEBIAN_VERSION}
		DEFAULT_LIBS_ROOT=${CROSS_BASE}/cross-libs-arm-neon-${DEBIAN_VERSION}
		DEBIAN_ARCH=armel
		ARCH=armv7l
		;;

	nofp)
		TOOLCHAIN=${BASEDIR}/meta/toolchain/cmake-toolchain-arm-linux-nofp.cmake
		DEFAULT_ROOT=${CROSS_BASE}/cross-root-arm-nofp-${DEBIAN_VERSION}
		DEFAULT_LIBS_ROOT=${CROSS_BASE}/cross-libs-arm-nofp-${DEBIAN_VERSION}
		DEBIAN_ARCH=armel
		ARCH=armv5tel
		;;

	armhf)
		TOOLCHAIN=${BASEDIR}/meta/toolchain/cmake-toolchain-arm-linux-armhf.cmake
		DEFAULT_ROOT=${CROSS_BASE}/cross-root-arm-armhf-${DEBIAN_VERSION}
		DEFAULT_LIBS_ROOT=${CROSS_BASE}/cross-libs-arm-armhf-${DEBIAN_VERSION}
		DEBIAN_ARCH=armhf
		ARCH=armv7l
		;;


	--help)
		grep -E '^#([^#!]|$)' $0
		exit
		;;
	*)
		echo "Not gonna work.  You gotta say \"neon\" or \"nofp\"."
		exit 1
		;;
esac


DIR_SUFFIX=""
if [ -n "$2" ]; then
	DIR_SUFFIX="-$2"
	DIR_SUFFIX_SET=1
else
	DIR_SUFFIX_SET=0
fi

if [ -n "$ROOT" ]; then
	ROOT_SET=1
else
	ROOT_SET=0
	ROOT="${DEFAULT_ROOT}${DIR_SUFFIX}"
fi
PREFIX="${ROOT}/usr/local"

if [ -n "$LIBS_ROOT" ]; then
	LIBS_ROOT_SET=1
else
	LIBS_ROOT_SET=0
	LIBS_ROOT="$DEFAULT_LIBS_ROOT"
fi
LIBS_PREFIX="${LIBS_ROOT}/usr/local"

case "${ROOT_SET}-${LIBS_ROOT_SET}-${DIR_SUFFIX_SET}" in
	0-0-0)
		BUILD_LIBS=1
		;;
	0-0-1)
		BUILD_LIBS=0
		;;
	0-1-0)
		BUILD_LIBS=1
		;;
	0-1-1)
		BUILD_LIBS=1
		;;
	1-0-0)
		BUILD_LIBS=0
		;;
	1-0-1)
		BUILD_LIBS=0
		;;
	1-1-0)
		BUILD_LIBS=1
		;;
	1-1-1)
		BUILD_LIBS=1
		;;
	*)
		echo "BUG: Unknown combination of $ROOT, $LIBS_ROOT, and directory suffix."
		exit 1
		;;
esac


BUILD=${BUILD:-RelWithDebInfo}
echo "Building a $BUILD build for $ARCH"

### Install libs for device image
ROOT_BUILD=build-$ARCH-$TARGET
rm -rf "$ROOT_BUILD"
mkdir "$ROOT_BUILD"
cd "$ROOT_BUILD"

export DEBIAN_ROOT=${CROSS_BASE}/debian-$DEBIAN_VERSION-root-$DEBIAN_ARCH-build
export LIBS_ROOT
export ROOT

mkdir -p "$ROOT/usr/lib/arm-linux-gnueabi" "$ROOT/usr/local/lib"
mkdir -p "$LIBS_ROOT/usr/lib/arm-linux-gnueabi" "$LIBS_ROOT/usr/local/lib"

cmake \
	-D CMAKE_TOOLCHAIN_FILE=${TOOLCHAIN} \
	-D CMAKE_INSTALL_PREFIX=${PREFIX} \
	-D CMAKE_BUILD_TYPE=${BUILD} \
	-D INSTALLDIR=${ROOT} \
	..

make -j$NCPUS
make -j$NCPUS install

cd ..

### Install libs and headers for local compilation
if [ "$BUILD_LIBS" -eq 1 ]; then
	LIBS_BUILD=build-$ARCH-$TARGET-libs
	rm -rf "$LIBS_BUILD"
	mkdir "$LIBS_BUILD"
	cd "$LIBS_BUILD"
	
	cmake \
		-D CMAKE_TOOLCHAIN_FILE=${TOOLCHAIN} \
		-D CMAKE_INSTALL_PREFIX=${LIBS_PREFIX} \
		-D CMAKE_BUILD_TYPE=Release \
		-D NL_INSTALL_HEADERS=true \
		-D INSTALLDIR=${LIBS_ROOT} \
		..

	make -j$NCPUS
	make -j$NCPUS install

	# Install into cross-package root for building other packages e.g. knd
	cmake \
		-D CMAKE_TOOLCHAIN_FILE=${TOOLCHAIN} \
		-D CMAKE_INSTALL_PREFIX=${DEBIAN_ROOT}/usr/local \
		-D CMAKE_BUILD_TYPE=Release \
		-D NL_INSTALL_HEADERS=true \
		-D INSTALLDIR=${DEBIAN_ROOT} \
		..

	make -j$NCPUS
	sudo make -j$NCPUS install

	cd ..
fi
