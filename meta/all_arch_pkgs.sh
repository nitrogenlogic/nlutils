#!/bin/sh
# Script to build Debian packages for all packaged architectures.
# Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
#
# The PKGDIR variable may be used to specify an output directory.  Packages
# will be placed into architecture-specific subdirectories of the output
# directory.

# List of Debian architecture names for which to build packages
ARCHLIST="${ARCHLIST:-i386 amd64}"

# Project directory
BASEDIR="$(readlink -m "$(dirname "$0")/..")"

# Default package output directory
BASE_PKGDIR="${PKGDIR:-/tmp}"

set -e -u

for ARCH in $ARCHLIST; do
	printf "\n\033[32mBuilding packages for \033[1m$ARCH\033[0m\n"
	ARCH=$ARCH PKGDIR="$BASE_PKGDIR/$ARCH" "$BASEDIR/meta/cross_pkg.sh"
done
