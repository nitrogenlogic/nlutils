# Package building helper script, using checkinstall.  Based on earlier package
# building scripts for nlutils and the logic system.  Scripts should be located
# one directory down from their project's top-level directory.  Set necessary
# variables, then source this file to build a package.
#
# Some parts of the script may not work with directory names with spaces.
#
# Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
#
# Required variables:
#   NAME - Short name of the project (e.g. "nlutils")
#   PKGNAME - Name of the package (e.g. "libnlutils")
#   DESCRIPTION - Description of the package
#   VERSION - Complete version number of the package (typically of the form x.y.z-r)
#
# One can also set the following variables:
#   MAINTAINER - maintainer e-mail address (defaults to "support@nitrogenlogic.com")
#   PKGGROUP - package group (defaults to "Nitrogen Logic")
#   PKGDIR - output directory (defaults to "/tmp")
#   PKGEXCLUDE - directories to exclude, comma-separated (defaults to "/home,/opt,/var/www,/etc/sudoers.d")
#   PKGDEPS - package dependencies (defaults to "")
#   INSTALL_CMD - command to install the code (passed to checkinstall, defaults to "make -C $BUILDDIR install")
#   BASEDIR - base directory for the project (defaults to expansion of "$0/..")
#   BUILDDIR - build directory for compiling code if default CMake is used (defaults to "$BASEDIR/build-pkg-$VERSION")
#   TESTS - if set to 0, tests will not be run
#
# Optional functions (defaults to no tests, CMake build within project dir):
#   build_code() - Commands to build the project.
#   test_code() - Commands to test the project.  Tests must pass before building a package.


# Build script for a typical Nitrogen Logic project's CMake setup.
cmake_build()
{
	# Set up build dir
	mkdir -p "$BUILDDIR"
	cd "$BUILDDIR"
	cmake -DCMAKE_INSTALL_PREFIX=/usr -DNL_PACKAGE=1 "$BASEDIR"
	make clean
	make -j$NCPUS
}

# Displays the command to be run on the terminal, then runs it.
show_run()
{
	printf "\n\033[0;34mRunning \033[1m"
	printf "'%s' " "$@"
	printf "\033[0m\n"

	"$@"
}

# Commands to build the package.
create_package()
{
	printf "\n\n\n\033[1;34mCompiled version $VERSION, running checkinstall to build package\033[0m\n\n"
	mkdir -p "$PKGDIR"

	ver=$(echo $VERSION | cut -d '-' -f 1)
	rel=$(echo $VERSION | cut -d '-' -f 2)

	# Temp dir will be left behind if something fails
	PKGTEMP=$(mktemp -d --tmpdir nlpk.XXXXXX)

	# Build package
	rm -f description-pak
	printf "$DESCRIPTION\n\n" | show_run sudo checkinstall \
		-D --install=no --reset-uids=yes --nodoc --backup=no \
		--strip=no --stripso=no \
		--maintainer="$MAINTAINER" --pkgsource="/dev/null" \
		--pkggroup="$PKGGROUP" --pkglicense="Proprietary" \
		--pkgversion="$ver" --pkgrelease="$rel" --pkgname="$PKGNAME" \
		--deldesc --deldoc --delspec --fstrans=yes --pakdir="$PKGTEMP" \
		--requires="$PKGDEPS" --exclude="$PKGEXCLUDE" \
		sh -c "$INSTALL_CMD"

	printf "\033[35m"
	cp -v "$PKGTEMP"/* "${PKGDIR}/"
	printf "\033[0m"

	# Clean up
	rm -f description-pak backup-*-pre-*
	rm -rf "$PKGTEMP/"
}


if [ -z "$NAME" -o -z "$PKGNAME" -o -z "$DESCRIPTION" -o -z "$VERSION" ]; then
	echo "Required variables missing"
	exit 1
fi


# Fail fast
set -e -u
trap '_Zret=$?; [ $_Zret -ne 0 ] && printf "\033[1;31mFailed pkg with status $_Zret\033[0m\n"; exit $_Zret' EXIT


# Configuration vars
MAINTAINER="${MAINTAINER:-"support@nitrogenlogic.com"}"
PKGGROUP="${PKGGROUP:-"Nitrogen Logic"}"
PKGDIR="${PKGDIR:-/tmp}"
PKGEXCLUDE="${PKGEXCLUDE:-"/home,/opt,/var/www,/etc/sudoers.d"}"
PKGDEPS="${PKGDEPS:-}"

# System info vars
NCPUS="$(grep -i 'processor.*:' /proc/cpuinfo | wc -l)"
[ "$NCPUS" = "0" ] && NCPUS=8

# Build vars
BASEDIR="${BASEDIR:-"$(readlink -m "$(dirname "$0")/..")"}"
BUILDDIR="${BUILDDIR:-"$BASEDIR/build-pkg-$VERSION"}"
INSTALL_CMD="${INSTALL_CMD:-"make -C $BUILDDIR install"}"


printf "\n\033[36mBuilding \033[1m$PKGNAME\033[0;36m for \033[1m$NAME\033[0;36m in \033[1m$PKGDIR\033[0m\n"
printf "\033[36m\tusing \033[1m$BUILDDIR\033[0m\n"
printf "\033[36m\tfrom \033[1m$BASEDIR\033[0m\n"
printf "\033[36m\tversion \033[1m$VERSION\033[0m\n"
printf "\033[36m\twith \033[1m$NCPUS\033[0;36m CPUs\033[0m\n"


# Compile code
# See https://stackoverflow.com/questions/14447555
if [ "$(command -v build_code)" = "build_code" ]; then
	printf "\n\033[1;34mCompiling project using build_code()\033[0m\n"
	cd "$BASEDIR"
	build_code || exit 1
else
	printf "\n\033[1;34mCompiling project using CMake defaults\033[0m\n"
	cmake_build || exit 2
fi

# Run tests
if [ "${TESTS:-1}" '!=' 0 -a "$(command -v test_code)" = "test_code" ]; then
	printf "\n\033[1;33mRunning project tests\033[0m\n"
	test_code || exit 3
fi

# Build package
create_package
