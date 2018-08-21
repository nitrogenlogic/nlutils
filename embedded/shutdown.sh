#!/bin/bash
# Shutdown from web interface
# Copyright (C)2011 Mike Bourgeous.  Released under AGPLv3 in 2018.

if [ "`id -u`" -ne 0 ]; then
	echo "Must be root to shut down (report this as a bug)." 1>&2
	echo "Running as user: `id -un`." 1>&2
	echo "Running as group: `id -gn`." 1>&2
	exit 1
fi

pgrphack /bin/bash -c '(sleep 2 ; /sbin/shutdown -h now & < /dev/null > /dev/null 2>&1' < /dev/null > /dev/null 2>&1 &
/sbin/shutdown -h now
