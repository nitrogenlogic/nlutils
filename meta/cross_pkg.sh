#!/bin/bash
# Script to build packages in a Debian cross-compilation root (e.g. for i386).
# Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.

# Debian architecture name
ARCH=${ARCH:-armel}

# Debian release name
RELEASE=buster

# Project directory
BASEDIR="$(readlink -m "$(dirname "$0")/..")"

. "${BASEDIR}/tools/build_root_helper.sh"
