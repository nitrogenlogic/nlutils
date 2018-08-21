#!/bin/sh
# Script to generate a Debian package for nlutils.
# Copyright (C)2011-2015 Mike Bourgeous.  Released under AGPLv3 in 2018.

# Configuration vars
NAME="nlutils"
PKGNAME="libnlutils"
DESCRIPTION="Nitrogen Logic utility library"
PKGDEPS=""

# Build vars
BASEDIR="$(readlink -m "$(dirname "$0")/..")"
VER=$(grep 'set(NLUTILS_VERSION' "${BASEDIR}/CMakeLists.txt" | egrep -o '[0-9]+\.[0-9]+\.[0-9]+')
REL=$(($(cat "$BASEDIR/meta/_RELEASE") + 1))
VERSION="$VER-$REL"


test_code()
{
	cd "$BUILDDIR/tests"
	./tests.sh
}


. "$BASEDIR/tools/pkg_helper.sh"


# Save bumped release number
printf "\nBuild complete; saving release number\n"
echo -n $REL > "$BASEDIR/meta/_RELEASE"
git commit -m "Build package $VERSION" "$BASEDIR/meta/_RELEASE"
