#!/bin/sh
# Common functions used by logic controller CGI scripts
# Copyright (C)2010 Mike Bourgeous.  Released under AGPLv3 in 2018.

# regex for use by sed to identify optionally-quoted strings
STRING_REGEX='\("\(\(\\.\|[^"]\)*\("\|$\)\)\|[^ 	]\+\)'
QUOTED_REGEX='"^\(.*\)$"'

# basic_html_escape < text > mostly_safe_html
basic_html_escape()
{
	sed -e 's/&/\&amp;/g' | sed -e 's/</\&lt;/g' | sed -e 's/>/\&gt;/g' | sed -e 's/"/\&quot;/g'
}

# strip_quotes < text > text_without_leading_and_trailing_quotes
strip_quotes()
{
	# TODO: Unescape internal quotes
	sed -e "s/$QUOTED_REGEX/\1/"
}

# connect < graph_commands > output 2>errors
connect()
{
	nc -q2 localhost 14309 2>/dev/null || echo No Design Running 1>&2
}

# do_graph_command "single graph command" > resultline 2>errors
do_graph_command()
{
	(echo "$1"; echo "bye") | connect | head -n1
}

# set_value objid paramindex value
set_value()
{
	do_graph_command "set $1,$2,$3"
}

# print_value objid index
# echoes the value, or nothing if objid/index is invalid
print_value()
{
	LINE=$(do_graph_command "get $1,$2" | sed -e 's/OK - \([^- ]\) - \(.*\)/\1 - \2/')
	TYPE=${LINE%% - *}
	VALUE=${LINE#* - }

	echo $VALUE | strip_quotes | basic_html_escape
}

# api_version > version (no newline after)
api_version()
{
	([ -f /var/lock/logicsystem/firmware.lock ] && echo "Firmware update in progress" || logicplugins --version) | tr -d '\r\n'
}

# build_number > number (no newline after)
build_number()
{
	. /etc/logicsystem_build > /dev/null 2>&1 || FW_BUILDNO='[unknown]'
	echo -n $FW_BUILDNO
}

# build_date > date (no newline after)
build_date()
{
	. /etc/logicsystem_build > /dev/null 2>&1 || FW_DATE='[unknown]'
	echo -n $FW_DATE
}

# build_year > year (no newline after)
build_year()
{
	. /etc/logicsystem_build > /dev/null 2>&1 || FW_DATE='[unknown]'
	echo -n $FW_DATE | cut -d '-' -f 1
}

# text_to_html < text > html
text_to_html()
{
	while read HTML_LINE; do
		echo $HTML_LINE | basic_html_escape | sed -e 's/	/&nbsp;&nbsp;&nbsp;&nbsp;/g'
		echo "<br>"
	done
}


# stdin->stdout
conv_percent_and_plus()
{
	# Quite slow, no doubt...
	while read -n1 b; do
		case "$b" in
			%)
			read -n2 x
			echo -en "\x$x"
			;;

			+)
			printf " "
			;;

			*)
			echo -n "$b"
			;;
		esac
	done
}

# get_equals_value source key lead_chars separator_chars > value
get_equals_value()
{
	# Thanks to http://www.ffnn.nl/pages/articles/linux/cgi-scripting-tips-for-bash-or-sh.php
	# for the grep -o idea
	echo "$1" | grep -oE "(^|[$3$4])$2=[^$4]+" | cut -d '=' -f 2 | head -n1
}

# get_query_value source key > value
get_query_value()
{
	get_equals_value $1 $2 '?' '&' | conv_percent_and_plus
}

# get_query_int source key > value
get_query_int()
{
	echo "$1" | grep -oE "(^|[?&])$2=-?[0-9]+" | cut -d '=' -f 2 | head -n1
}


logo_div()
{
	echo "	<div id=\"logo\">"
	echo "		<a href=\"http://nitrogenlogic.com/\">Nitrogen Logic</a>"
	echo "		<div class=\"subtitle\">"
	echo -n "			"; api_version; echo -n ", build "; build_number; echo
	echo "		</div>"
	echo "	</div>"
}

title_div()
{
	echo "	<div id=\"title\">"
	echo "		$1"
	echo "		<div class=\"subtitle\">$2</div>"
	echo "	</div>"
}

footer_div()
{
	echo "<div id=\"footer\">"
	echo "	<div id=\"buildtime\">"
	echo -n "	Firmware built on "; build_date; echo
	echo "	</div>"
	echo "	<div id=\"copyright\">"
	echo "		&copy;$(build_year) Nitrogen Logic<br>"
	echo "	</div>"
	echo "	<div style=\"text-align: center\">"
	echo "		<a href=\"source_info\">Open source information</a>"
	echo "	</div>"
	echo "	<div class=\"clear\"></div>"
	echo "</div>"
}

POST_DATA=$([ ! -z "$CONTENT_LENGTH" ] && dd bs="$CONTENT_LENGTH" count=1)

if echo "$0" | grep "common.sh$" > /dev/null 2>&1; then
	echo "Content-Type: text/plain"
	echo
fi
