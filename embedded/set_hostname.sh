#!/bin/bash
# Host name changing script for web interface on Nitrogen Logic controllers
# Validates the host name, changes it in the various places it is stored,
# and restarts the Avahi service discovery daemon.
# Copyright (C)2010 Mike Bourgeous.  Released under AGPLv3 in 2018.


[ -z "$1" ] && echo "No hostname specified" 1>&2 && exit 1

HN=`echo -n "$1" | tr [[:upper:]] [[:lower:]] | tr -c [[:alnum:]-] - | head -c64`

if [ "`id -u`" -ne 0 ]; then
	echo "Must be root to change host name (report this as a bug)." 1>&2
	echo "Running as user: `id -un`." 1>&2
	echo "Running as group: `id -gn`." 1>&2
	exit 1
fi
if echo "$HN" | grep '^[^a-z0-9]' > /dev/null; then
	echo "Hostname must start with a letter or number." 1>&2
	exit 1
fi
if echo "$HN" | grep '[^a-z0-9]$' > /dev/null; then
	echo "Hostname must end with a letter or number." 1>&2
	exit 1
fi
if echo "$HN" | grep '[a-z]' > /dev/null; then
	echo Changing hostname from $(hostname) to "$HN"
	logger -t set_hostname Web interface changing hostname from $(hostname) to "$HN".
else
	echo "Hostname must contain at least one letter." 1>&2
	exit 1
fi

set -e

hostname "$HN"
# TODO: Delete a line containing the old hostname, add a line for the new one
sed -i -e "s/^127\.0\.1\.1.*/127.0.1.1	${HN}/" /etc/hosts
echo "$HN" > /etc/hostname
/etc/init.d/avahi-daemon restart > /dev/null

