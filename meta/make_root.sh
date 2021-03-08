#!/bin/bash
# Builds a Debian root image for the SheevaPlug, including nlutils.  May
# require installing mtd-utils from source, as at one point the Ubuntu version
# had a bug in its CRC calculation.
#
# Run from the nlutils base directory where crosscompile.sh is located.
#
# Useful references:
# http://blog.bofh.it/debian/id_265
# http://wiki.debian.org/EmDebian/CrossDebootstrap#QEMU.2BAC8-debootstrap_approach
#
# Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.

# Project directory
BASEDIR="$(readlink -m "$(dirname "$0")/..")"

# Debian architecture name
ARCH=${ARCH:-armel}

# Debian version name
RELEASE=${RELEASE:-stretch}

# Output file for root filesystem image
FSIMAGE="$HOME/devel/crosscompile/ubifs-${RELEASE}-${ARCH}.img"

# Output file for UBI image
UBI="$HOME/devel/crosscompile/ubi${ARCH%%armel}.img"

# UBI configuration file (will be overwritten and deleted)
UBICFG=`mktemp /tmp/ubi-${RELEASE}-${ARCH}-XXXXXXXX.cfg`

# Extra packages root_helper.sh will install
EXTRA_PACKAGES="\
watchdog,\
daemontools,\
ntp,\
ntpdate,\
vsftpd,\
lighttpd,\
conspy,\
u-boot-tools,\
openssh-server,\
openssh-client,\
mtd-utils,\
logrotate,\
cron\
"

printf "\033[1;33m\n\nBuilding base root\033[0m\n\n"
. "${BASEDIR}/tools/root_helper.sh"

printf "\033[1;33m\n\nBuilding build root\033[0m\n\n"
PACKAGE=0 ROOT_SUFFIX=${ROOT_SUFFIX:+$ROOT_SUFFIX-}build . "${BASEDIR}/tools/build_root_helper.sh"

### Set hostname and other settings
printf "\033[1;33m\n\nConfiguring target system settings\033[0m\n\n"
echo logic-controller | pipetoroot "/etc/hostname"
echo LANG=C | pipetoroot "/etc/default/locale"
echo | pipetoroot "/etc/udev/rules.d/75-persistent-net-generator.rules"
sudo rm -f "${ROOTPATH}/etc/udev/rules.d/70-persistent-net.rules"
sudo rm -f "${ROOTPATH}/dev/ttyS0"
sudo mknod "${ROOTPATH}/dev/ttyS0" c 4 64
printf "nosoup4u\nnosoup4u\n" | sudo chroot "${ROOTPATH}" passwd root

pipetoroot "/etc/hosts" <<EOF
127.0.0.1 localhost
127.0.1.1	logic-controller
EOF

pipetoroot "/etc/network/interfaces" <<EOF
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet dhcp
EOF

pipetoroot "/etc/inittab" <<EOF
# inittab based on Debian inittab, with the following log line:
# \$Id: inittab,v 1.91 2002/01/25 13:35:21 miquels Exp \$

id:2:initdefault:

si::sysinit:/etc/init.d/rcS

~~:S:wait:/sbin/sulogin

l0:0:wait:/etc/init.d/rc 0
l1:1:wait:/etc/init.d/rc 1
l2:2:wait:/etc/init.d/rc 2
l3:3:wait:/etc/init.d/rc 3
l4:4:wait:/etc/init.d/rc 4
l5:5:wait:/etc/init.d/rc 5
l6:6:wait:/etc/init.d/rc 6
z6:6:respawn:/sbin/sulogin

pf::powerwait:/etc/init.d/powerfail start
pn::powerwait:/etc/init.d/powerfail now
po::powerwait:/etc/init.d/powerfail stop

T0:2345:respawn:/sbin/getty -L ttyS0 115200 linux
EOF

pipetoroot "/etc/default/thttpd" <<EOF
ENABLED=yes
EOF


### Copy nlutils into root
case $ARCH in
	armel)
		nlutils_arch=nofp
		;;
	armhf)
		nlutils_arch=armhf
		;;
	*)
		nlutils_arch=$ARCH
		;;
esac

echo "Building and installing nlutils"
rm -rf "${ROOTPATH}-nlutils"
ROOT="${ROOTPATH}-nlutils" "${BASEDIR}/crosscompile.sh" $nlutils_arch
sudo cp -Rdfv "${ROOTPATH}-nlutils"/* "$ROOTPATH"
sudo chown root:www-data "$ROOTPATH"/opt/nitrogenlogic/util/*
sudo chmod 4750 "$ROOTPATH"/opt/nitrogenlogic/util/*
sudo chown root:root "${ROOTPATH}/etc/sudoers.d/sudo_nl_web"
sudo chmod 0440 "${ROOTPATH}/etc/sudoers.d/sudo_nl_web"
sudo chroot ${ROOTPATH} /sbin/ldconfig


### Clean up logs and binfmt helper
sudo find "${ROOTPATH}/var/log" -type f -print -exec sh -c "dd if=/dev/null of={} &> /dev/null" \;
sudo rm "${ROOTPATH}/${QEMU}"

if [ "$PACKAGE" != '0' ]; then
	### Build UBIFS and UBI images
	echo "Building UBI image"
	cat > "$UBICFG" <<EOF_UBI
[rootfs]
mode=ubi
image=${FSIMAGE}
vol_id=0
vol_size=200MiB
vol_type=dynamic
vol_name=rootfs
vol_flags=autoresize
EOF_UBI

	# For some unfathomable reason, mkfs.ubifs removed the option to force the root
	# inode's permissions to root:root 755.
	sudo chown root:root "$ROOTPATH"
	sudo chmod 755 "$ROOTPATH"

	DEBIAN_FRONTEND=noninteractive sudo apt-get install --no-install-recommends -y mtd-utils
	sudo mkfs.ubifs -v -r "$ROOTPATH" -m 2048 -e 129024 -c 4096 -o "$FSIMAGE" -x "favor_lzo"
	sudo ubinize -v -o "$UBI" -m 2048 -p 128KiB -s 512 "$UBICFG"
	sudo chown `id -u`:`id -g` "$UBI" "$FSIMAGE"
	rm "$UBICFG"

	echo "UBI image built in '$UBI'"
fi
