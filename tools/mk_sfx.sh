#!/bin/bash
# Creates a base64-encoded self-extracting tar archive.  The extracting system
# must have GNU tar, GNU coreutils (for base64), and bzip2 installed.
# Copyright (C)2011 Mike Bourgeous.  Released under AGPLv3 in 2018.

function create_archive()
{
	set -e
	echo '#!/bin/sh'
	echo 'cat << _NLFW_EOF_ | base64 -d | tar -jxp'
	tar -jcv -- "$@" | base64 -w 120
	RESULT=${PIPESTATUS[0]}
	echo '_NLFW_EOF_'
	return $RESULT
}

if [ "$1" = "" -o "$2" = "" ]; then
	echo "Usage: $0 output_file input_files..."
	echo "Use '-' for output_file to send to stdout"
	exit 1
fi

OUTFILE="$1"

set -e

shift

if [ "$OUTFILE" = "-" ]; then
	create_archive "$@"
else
	create_archive "$@" > "$OUTFILE"
fi

