# Debian cross-compilation root helper script, using debootstrap and qemu.
# Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
#
# Command line arguments:
#   --rebuild - If $1 or $2, rebuilds root from scratch instead of updating
#
# Required variables:
#   ARCH - Debian architecture name (i386, amd64, armel, armhf)
#   RELEASE - Debian release to download (e.g. squeeze, lenny, stable, testing)
#
# Optional variables:
#   ROOT_BASE - base location of output root (defaults to $HOME/devel/crosscompile)
#   BASEDIR - base directory for the project (defaults to expansion of "$0/..")
#   ROOT_SUFFIX - string added to the end of the output root directory name
#   EXTRA_PACKAGES - list of extra packages to install into the root
#   $1 - if --rebuild, rebuilds the root image from scratch

if [ -z "$ARCH" -o -z "$RELEASE" ]; then
	echo "Required variables missing"
	exit 1
fi

# Debian mirror
if [ $RELEASE = "squeeze" ]; then
	MIRROR=http://archive.debian.org/debian/
else
	MIRROR=https://mirrors.kernel.org/debian/
fi

# Build vars
BASEDIR="${BASEDIR:-"$(readlink -m "$(dirname "$0")/..")"}"
ROOT_BASE="${ROOT_BASE:-$HOME/devel/crosscompile}"

# Directory in which to build root filesystem (omit trailing slash!)
ROOTPATH="${ROOT_BASE}/debian-${RELEASE}-root-${ARCH}${ROOT_SUFFIX:+-$ROOT_SUFFIX}"

# Base packages to install in the root
PACKAGES="\
debian-archive-keyring,\
debian-ports-archive-keyring,\
udev,\
kmod,\
netbase,\
net-tools,\
ifupdown,\
whiptail,\
gpgv,\
vim-tiny,\
locales,\
avahi-autoipd,\
avahi-daemon,\
avahi-utils,\
libnss-mdns,\
psmisc,\
kbd,\
sudo,\
lsof,\
usbutils,\
netcat-openbsd,\
libasound2,\
libavahi-compat-libdnssd1,\
ruby,\
libusb-1.0-0,\
libpng16-16,\
bzip2,\
uuid-runtime,\
wget,\
schedtool,\
isc-dhcp-client,\
iputils-ping,\
ca-certificates,\
curl,\
libevent-dev\
${EXTRA_PACKAGES:+,$EXTRA_PACKAGES}\
"

# Fail fast
set -e -u
trap '_Zret=$?; [ $_Zret -ne 0 ] && printf "\033[1;31mFailed root with status $_Zret\033[0m\n"; exit $_Zret' EXIT


# QEMU binary and uname architecture to use for $ARCH
case $ARCH in
	amd64)
		QEMU=/usr/bin/qemu-x86_64-static
		UNAME=linux64
		;;
	armel|armhf)
		QEMU=/usr/bin/qemu-arm-static
		UNAME=linux32
		;;
	*)
		QEMU=/usr/bin/qemu-$ARCH-static
		UNAME=linux32
		;;
esac


# Overwrites the file given in $1 from stdin using sudo.
pipeto()
{
	printf "\n\033[0;33mWriting $1\033[0m\n"
	sudo sh -c "cat > \"$1\""
}

# Overwrites the file in $ROOTPATH/$1 from stdin using sudo.
pipetoroot()
{
	pipeto "${ROOTPATH}/$1"
}


# Displays the command to be run on the terminal, then runs it.
show_run()
{
	printf "\n\033[0;34mRunning \033[1m"
	printf "'%s' " "$@"
	printf "\033[0m\n"

	"$@"
}

# Makes sure the filesystem that contains $ROOTPATH is mounted with the "dev"
# option (not nodev).
remount_with_dev()
{
	fromdir=$(pwd)

	# https://unix.stackexchange.com/questions/149660/mount-info-for-current-directory
	until findmnt . ; do
		cd ..
	done

	fs=$(pwd)

	printf "\n\033[1mMounting target filesystem ($fs) with \033[32mdev\033[0m\n"
	printf "\n\033[1;31mWarning:\033[0m it is recommended that you remount with nodev\n"
	echo "again later, if that option was enabled before."

	show_run sudo mount -i -o remount,dev "$fs"

	cd "$fromdir"
}

# Calls the given command with 'sudo chroot "$ROOTPATH" setarch "$UNAME"',
# preserving the TESTS and VALGRIND environment variables.
sudo_root()
{
	show_run sudo HOME=/root TESTS=${TESTS:-} VALGRIND=${VALGRIND:-} PKGDIR=${PKGDIR:-/tmp} chroot "${ROOTPATH}" setarch "$UNAME" -- "$@"
}


printf "\n\033[0;1mBuilding/updating root image\033[0;36m\n"
printf "\tin \033[1m'$ROOTPATH'\033[0;36m\n"
printf "\tfor \033[1m'$RELEASE'\033[0;36m\n"
printf "\ton \033[1m'$ARCH'\033[0;36m\n"
printf "\tusing \033[1m'$QEMU'\033[0m\n"


### Install prerequisites
printf "\n\033[1mInstalling required local packages, if any\033[0m\n"
DEBIAN_FRONTEND=noninteractive sudo apt-get install --no-install-recommends -y qemu-user-static debootstrap debian-archive-keyring debian-ports-archive-keyring


### Fix filesystem mount permissions to allow device files
remount_with_dev


### Download and unpack Debian packages
if [ ! -d "$ROOTPATH/etc/apt" -o "${1:-}" = "--rebuild" -o "${2:-}" = "--rebuild" ]; then
	printf "\n\033[1mBootstrapping with debootstrap\033[0m\n"
	sudo rm -rf "${ROOTPATH}"/{,.[^.]}*

	# Copy qemu
	sudo mkdir -p "${ROOTPATH}/usr/bin"
	sudo cp "$QEMU" "$ROOTPATH/$QEMU"

	# Copy release keys
	sudo mkdir -p "${ROOTPATH}/usr/share/keyrings/"
	sudo cp /usr/share/keyrings/debian-archive* "${ROOTPATH}/usr/share/keyrings/"

	# Run bootstrap
	sudo debootstrap --foreign --arch="$ARCH" --variant=minbase --components="main,contrib,non-free" --include="$PACKAGES" "$RELEASE" "$ROOTPATH" "$MIRROR"
	sudo_root /debootstrap/debootstrap --second-stage

	sudo_root /usr/bin/apt-get clean
	sudo find "$ROOTPATH/var/cache/apt/archives/" -type f -delete
else
	printf "\n\033[1mRoot directory ($ROOTPATH) already exists.  Skipping bootstrap.\033[0m\n"
	echo "Pass --rebuild as first argument to force bootstrap."
	sudo mkdir -p "${ROOTPATH}/usr/bin"
	sudo cp "$QEMU" "$ROOTPATH/$QEMU"
fi


### Set essential root settings
echo nameserver 8.8.8.8 | pipetoroot "/etc/resolv.conf"
pipetoroot "/etc/apt/sources.list" <<EOF
deb $MIRROR ${RELEASE} main contrib non-free
EOF


### Update packages
sudo_root /usr/bin/apt-get update -o Acquire::Check-Valid-Until=false
sudo_root sh -c "DEBIAN_FRONTEND=noninteractive /usr/bin/apt-get --no-install-recommends --force-yes -y dist-upgrade"
sudo_root sh -c "DEBIAN_FRONTEND=noninteractive /usr/bin/apt-get --no-install-recommends --force-yes -y install $(echo $PACKAGES | sed -e 's/,/ /g')"
sudo_root /usr/bin/apt-get clean
