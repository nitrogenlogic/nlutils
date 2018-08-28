#!/bin/bash
# Nitrogen Logic utility library tests
# Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.

# TODO: Extract a set of helper scripts that can be used for tests in other projects

SERVER_PID=0
PASSED=0
TOTAL=0
FAILTEXT=''
TMPFILE1=''
TMPFILE2=''
FIRMWARE01=''
FIRMWARE02=''
FIRMWARE02a=''
FIRMWARE02b=''
FIRMWARE02c=''
FIRMWARE_FAIL=''

# Starts a parallel server process, storing its PID in $SERVER_PID.  Waits two
# seconds for the server to start.  For use by any test that requires a
# parallel process to be running.
start_server()
{
	setsid "$@" &
	SERVER_PID=$!
	sleep 2
}

# Kills whatever process group is in $SERVER_PID.  For use by any test that
# requires a parallel process to be running.
kill_server()
{
	if [ -n "$SERVER_PID" -a "$SERVER_PID" -gt 1 ]; then
		echo "Killing test server PID -$SERVER_PID"

		kill -TERM -$SERVER_PID 2>/dev/null
		sleep 1
		kill -9 -$SERVER_PID 2>/dev/null

		SERVER_PID=0
	fi
}

showfails()
{
	if [ '!' -z "$FAILTEXT" ]; then
		printf "\e[1;31mFailures:\n\e[0;31m${FAILTEXT}\e[0m\n" >&2
	fi
}

cleanup()
{
	if [ $PASSED -eq $TOTAL ]; then
		printf "\n\n\e[0;32mAll \e[1;32m$PASSED\e[0;32m of \e[1;32m$TOTAL\e[0;32m tests \e[1;32mpassed\e[0;32m.\e[0m\n"
		EXITCODE=0
	elif [ $TOTAL -lt 0 ]; then
		printf "\n\e[0;31mTesting was interrupted (\e[0;1;32m$PASSED\e[0;32m of $((-$TOTAL)) passed\e[0;31m).\n" >&2
		EXITCODE=1
	else
		printf "\n\n\e[1;31m$(($TOTAL - $PASSED))\e[0;31m of \e[1;31m$TOTAL\e[0;31m tests failed (\e[0;1;32m$PASSED\e[0;32m passed\e[0;31m).\e[0m\n" >&2

		FAILED=$(($TOTAL - $PASSED))
		EXITCODE=$(($FAILED > 255 ? 255 : $FAILED))
	fi

	kill_server

	showfails

	[ "$DEBUG" != "1" ] && rm -f "$TMPFILE1" "$TMPFILE2" "$FIRMWARE01" "$FIRMWARE02" "$FIRMWARE02a" "$FIRMWARE02b" \
		"$FIRMWARE02c" "$FIRMWARE_FAIL"

	exit $EXITCODE
}

pass()
{
	printf "\e[1;32mPASS: \e[0;32m$*\e[0m\n"
	PASSED=$(($PASSED + 1))
}

fail()
{
	printf "\n\e[1;31mFAIL: \e[0;31m$*\e[0m\n" >&2
	FAILTEXT="${FAILTEXT}\\t$*\\n"

	if [ "$FAILFAST" = 1 ]; then
		FAILTEXT="${FAILTEXT}\\tAborted early due to FAILFAST=1\\n"
		exit 1
	fi
}

headline()
{
	echo
	printf "\e[1;35m%s\e[0m" "$*"
	echo
}

# Usage: check true/false message
# TODO: Record output of failed tests?
check()
{
	ret=$?

	case $1 in
		true)
			relation="-eq"
			;;
		false)
			relation="-ne"
			;;
		*)
			fail "Invalid first parameter to check()"
			exit 1
			;;
	esac

	shift

	TOTAL=$(($TOTAL + 1))
	if [ $ret -eq 77 ]; then
		fail "Valgrind error $ret for $*"
	elif [ $ret $relation 0 ]; then
		pass "Got $ret for $*"
	else
		fail "Got $ret for $*"
	fi
}

badsignal()
{
	fail "Got a fatal signal"
	TOTAL=$((-$TOTAL))
	exit 1
}

# Usage: runtest expected(true/false) message command [args...]
runtest()
{
	expected="$1"
	shift
	message="$1"
	shift
	cmd="$1"
	shift

	if [ -x /usr/bin/valgrind ]; then
		case "$VALGRIND" in
			0)
				"$cmd" "$@"
				;;

			helgrind|drd)
				/usr/bin/valgrind --tool=$VALGRIND --read-var-info=yes --num-callers=40 \
					--error-exitcode=77 -- "$cmd" "$@"
				;;

			*)
				/usr/bin/valgrind -q --error-exitcode=77 --leak-check=full --show-reachable=yes \
					--read-var-info=yes --track-origins=yes --track-fds=yes --num-callers=40 \
					--malloc-fill=a5 --free-fill=5a -- "$cmd" "$@"
				;;
		esac
	else
		printf "\033[1;33mNote: /usr/bin/valgrind is unavailable.\033[0m\n"
		"$cmd" "$@"
	fi

	check "$expected" "$message"
}

trap cleanup EXIT
trap badsignal SIGINT SIGTERM
umask 0077

cd "$(dirname "$0")"


# Test program execution functions
headline "Testing program execution functions"
runtest true 'Execution function tests' \
	./exec_test

# Test time functions
headline "Testing time-related functions"
runtest true 'Time function tests' \
	./time_test

# Test memory management functions
headline "Testing memory management functions"
runtest true 'Memory management tests' \
	./mem_test

# Test string functions
headline "Testing string functions"
runtest true 'String function tests' \
	./string_test
runtest true 'String escape tests' \
	./escape_test


# Test FIFO (struct nl_fifo) functions
headline "Testing nl_fifo functions"
runtest true 'FIFO tests' \
	./fifo_test


# Test associative array functions
headline 'Testing hash table/associative array functions'
runtest true 'Hash table tests' \
	./hash_test


# Test URL functions
headline 'Testing URL functions'
runtest true 'URL function tests' \
	./url_test


# Test URL request processing functions
headline 'Testing URL request functions'
start_server ./url_req/url_req_server.rb
runtest true 'URL request function tests' \
	./url_req/url_req_test
kill_server


# Test variant functions
headline "Testing variant type functions"
runtest true 'Variant clamping tests' \
	./variant/clamp_test
runtest true 'Variant comparison tests' \
	./variant/compare_test
runtest true 'Variant to string tests' \
	./variant/to_string_test
runtest true 'Sized raw data tests' \
	./variant/data_test


# Test key-value pair parsing functions
headline "Testing key-value pair parsing functions"
runtest true 'Key-value pair parsing tests' \
	./kvp_test


# Test threading functions
headline "Testing thread-related functions"
runtest true 'Thread-related function tests' \
	./thread_test


# Test network-related functions
headline "Testing network-related functions"
runtest true 'Network-related function tests' \
	./net_test


# Test firmware generation
headline "Testing firmware creation"

FIRMWARE01=$(mktemp --tmpdir firmware01.XXXXXX.nlfw)
VALGRIND=0 runtest true 'Generate version 3 firmware' \
	../tools/mk_firmware.sh firmware/success.sh "$FIRMWARE01"

FIRMWARE02=$(mktemp --tmpdir firmware02.XXXXXX.nlfw)
VALGRIND=0 runtest true 'Generate version 3 firmware without architecture check' \
	../tools/mk_firmware.sh firmware/success.sh "$FIRMWARE02" ""

FIRMWARE02a=$(mktemp --tmpdir firmware02a.XXXXXX.nlfw)
VALGRIND=0 runtest true 'Generate version 3 firmware without firmware name' \
	../tools/mk_firmware.sh firmware/success.sh "$FIRMWARE02a" "$(uname -m)"

FIRMWARE02b=$(mktemp --tmpdir firmware02b.XXXXXX.nlfw)
VALGRIND=0 runtest true 'Generate version 3 firmware with firmware name but no arch' \
	../tools/mk_firmware.sh firmware/success.sh "$FIRMWARE02b" "" "Test Firmware (no arch)"

FIRMWARE02c=$(mktemp --tmpdir firmware02c.XXXXXX.nlfw)
VALGRIND=0 runtest true 'Generate version 3 firmware with firmware name and arch' \
	../tools/mk_firmware.sh firmware/success.sh "$FIRMWARE02c" "$(uname -m)" "Test Firmware"

FIRMWARE_FAIL=$(mktemp --tmpdir firmware_fail.XXXXXX.nlfw)
VALGRIND=0 runtest true 'Generate version 3 firmware that always fails' \
	../tools/mk_firmware.sh firmware/fail.sh "$FIRMWARE_FAIL" ""

# Test firmware extraction
headline "Testing firmware extraction"
runtest false 'Extract an invalid file' \
	../programs/do_firmware firmware/success.sh

runtest true 'Extract version 3 firmware' \
	../programs/do_firmware $FIRMWARE01

runtest true 'Extract version 3 firmware without architecture check' \
	../programs/do_firmware $FIRMWARE02

runtest true 'Extract version 3 firmware without firmware name' \
	../programs/do_firmware $FIRMWARE02a

runtest true 'Extract version 3 firmware with firmware name but no arch' \
	../programs/do_firmware $FIRMWARE02b

runtest true 'Extract version 3 firmware with firmware name and arch' \
	../programs/do_firmware $FIRMWARE02c

runtest false 'Extract version 3 firmware that is expected to fail' \
	../programs/do_firmware $FIRMWARE_FAIL

# Test debugging functions
headline "Testing debugging-related functions"
runtest true 'Debugging-related function tests' \
	./debug_test
