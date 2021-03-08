# Helper script for building a Debian root and then cross-compiling packages.
# Expects projects to have a Makefile, a meta/make_pkg.sh script, and .git/.
# Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
#
# Refreshes or rebuilds the cross-compilation root, clones the project into a
# temporary directory in the root, then builds the project's packages.
#
# TODO: Convert from checkinstall to traditional Debian package process
#
# See root_helper.sh for required environment variables ARCH and RELEASE.
#
# Optional variables (also see root_helper.sh):
#   PKGDIR - output directory (defaults to "/tmp")
#   EXTRA_PACKAGES - packages to add to the cross-compilation root image
#   LOCAL_BUILD - command for local build and installation (defaults to "make && make install")
#   $1 - if --rebuild, rebuilds the root image from scratch

# Project directory
BASEDIR="$(readlink -m "$(dirname "$0")/..")"

# Project name
PROJECT="$(basename "$BASEDIR")"

# Suffix for build root directory name
ROOT_SUFFIX=${ROOT_SUFFIX:-build}

# Package output path
PKGDIR="${PKGDIR:-/tmp}"
ORIG_PKGDIR="$PKGDIR"

# Local installation command
LOCAL_BUILD="${LOCAL_BUILD:-make && make install}"

# Current git branch
BRANCH="$(git branch | grep '^\*' | cut -d ' ' -f 2-)"

# Required development packages
EXTRA_PACKAGES="\
build-essential,\
clang,\
cmake,\
git,\
checkinstall,\
ruby,\
libudev-dev,\
libevent-dev,\
libusb-1.0-0-dev,\
${EXTRA_PACKAGES:+,$EXTRA_PACKAGES}
"


# Update the root image
if [ -r "${BASEDIR}/tools/root_helper.sh" ]; then
	. "${BASEDIR}/tools/root_helper.sh"
elif [ -r /usr/local/share/nlutils/root_helper.sh ]; then
	. /usr/local/share/nlutils/root_helper.sh
elif [ -r /usr/share/nlutils/root_helper.sh ]; then
	. /usr/share/nlutils/root_helper.sh
else
	printf "\033[1;31mCan't find nlutils cross-root helper script\033[0m\n"
	exit 1
fi
sudo_root git config --global user.name "$ARCH-$RELEASE cross-build"
sudo_root git config --global user.email "support@nitrogenlogic.com"


# Set up the build path
CLONEPATH="/tmp/nlutils-${PROJECT}-pkg"
printf "\033[36mBuilding $BRANCH branch of \033[1m${PROJECT}\033[0m\n"
sudo_root <<-EOF
rm -rf "$CLONEPATH"
mkdir -p "$CLONEPATH"
EOF

# Copy source code
sudo git -C "${ROOTPATH}/${CLONEPATH}" clone --single-branch "${BASEDIR}" "$PROJECT"

# Install the project into /usr/local for use by other projects (TODO: just use the package)
sudo_root sh -c "cd ${CLONEPATH}/${PROJECT} && sh -c \"$LOCAL_BUILD\""

# Build the package
PKGDIR="${CLONEPATH}" sudo_root "${CLONEPATH}/${PROJECT}/meta/make_pkg.sh"
PKGDIR="$ORIG_PKGDIR"

# Copy release version update back into project directory
GITREMOTE="build-$$-$(date --iso-8601=ns | tr -cd 0-9-)"
show_run git remote add -t "$BRANCH" "$GITREMOTE" "${ROOTPATH}/${CLONEPATH}/${PROJECT}"
show_run git pull "$GITREMOTE" "$BRANCH"
show_run git remote remove "$GITREMOTE"

# Move package files to their final destination and clean up
printf "\n\033[35mCopying package files to $PKGDIR/\033[0m\n"
mkdir -p "$PKGDIR/"
cp -v -- "${ROOTPATH}${CLONEPATH}"/*.deb "$PKGDIR/"
sudo_root rm -rf "$CLONEPATH"
