#!/bin/bash
# Generates a .nlfw image from a bash or sh script.
# Copyright (C)2011-2018 Mike Bourgeous.  Released under AGPLv3 in 2018.

# Note: firmware updates are basically intentional remote code execution.  All
# rudimentary firmware authenticity verification was removed for public
# release, so actual use of this released firmware format is not advised.

set -e

function sha1_clean()
{
	sha1sum | egrep -o '[0-9a-f]{40}' || (echo 'sha1sum failed' >&2; exit 1)
}

if [ "$1" = "" -o "$2" = "" ]; then
	echo "Usage: $0 source destination [arch [firmware_name]]"
	echo "Generates a v3 firmware file.  Versions 1 and 2 are deprecated."
	echo "Architecture string, if present, must match \`uname -m\` to unpack."
	echo "Firmware name is an optional string added to the firmware file."
	exit 1
fi

VER=03

if [ "${#3}" -gt 63 ]; then
	echo "Architecture string '$3' is too long -- 63 characters max."
	exit 1
elif [ -n "$3" ]; then
	echo "Architecture '$3' specified." >&2
	ARCH="$3"
else
	ARCH=""
fi

if [ "${#4}" -gt 63 ]; then
	echo "Firmware name string '$4' is too long -- 63 characters max."
	exit 1
elif [ -n "$3" ]; then
	echo "Firmware name is '$4'." >&2
	NAME="$4"
else
	NAME=""
fi

HEAD_SH=$(head -c9 "$1")
HEAD_BASH=$(head -c11 "$1")

if [ ! \( "$HEAD_SH" = "#!/bin/sh" -o "$HEAD_BASH" = "#!/bin/bash" \) ]; then
	echo "Source file must start with #!/bin/sh or #!/bin/bash."
	exit 1
fi

printf "NLFW_${VER}\n" > "$2"
printf "$ARCH\n" >> "$2"
printf "$NAME\n" >> "$2"
sha1_clean < "$1" >> "$2"
gzip < "$1" >> "$2"

exit 0
