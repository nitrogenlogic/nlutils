#!/bin/bash
# Increments version numbers.  Used manually or by build scripts.
# Copyright (C)2011 Mike Bourgeous.  Released under AGPLv3 in 2018.

function usage()
{
	echo "Usage: $0 [--release|--minor|--major] [--nocommit]"
	echo "--nocommit must come last"
	exit 1
}

VER=$(grep NLUTILS_VERSION CMakeLists.txt | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')

MAJOR=$(echo $VER | cut -d . -f 1)
MINOR=$(echo $VER | cut -d . -f 2)
RELEASE=$(echo $VER | cut -d . -f 3)

echo Major: $MAJOR Minor: $MINOR Release: $RELEASE

if [ $# -gt 2 -o \( $# -gt 1 -a "$1" = "--nocommit" \) ]; then
	usage
fi

case "$1" in
	"--release" | "--nocommit" | "")
		echo "Incrementing release version from $RELEASE to $(($RELEASE + 1))"
		RELEASE=$(($RELEASE + 1))
		VERNAME=release
		;;
	"--minor")
		echo "Incrementing minor version from $MINOR to $(($MINOR + 1))"
		RELEASE=0
		MINOR=$(($MINOR + 1))
		VERNAME=minor
		;;
	"--major")
		echo "Incrementing major version from $MAJOR to $(($MAJOR + 1))"
		RELEASE=0
		MINOR=0
		MAJOR=$(($MAJOR + 1))
		VERNAME=major
		;;
	*)
		echo "Unknown option: $1"
		usage
		;;
esac

if [ "$1" = "--nocommit" -o "$2" = "--nocommit" ]; then
	echo "Not committing change to git."
fi

VER=${MAJOR}.${MINOR}.${RELEASE}
echo "New version number: $VER"
echo -n "Are you sure (y/N)? "
read REPLY
if [ "$REPLY" != "y" -a "$REPLY" != "yes" -a "$REPLY" != "Y" ]; then
	echo "Aborting."
	exit 2
fi

sed -i CMakeLists.txt -e 's/\(set(NLUTILS_VERSION[[:space:]]\+\)[0-9]\+\.[0-9]\+\.[0-9]\+)/'"\\1${MAJOR}.${MINOR}.${RELEASE})/"
sed -i CMakeLists.txt -e 's/\(set(NLUTILS_SO_VERSION[[:space:]]\+\)[0-9]\+)/'"\\1${MINOR})/"
echo -n 0 > meta/_RELEASE

if [ "$1" != "--nocommit" -a "$2" != "--nocommit" ]; then
	git commit meta/_RELEASE CMakeLists.txt \
		-e -m "Incremented $VERNAME version and reset build number."
fi
