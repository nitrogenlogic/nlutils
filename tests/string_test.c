/*
 * Tests string utility functions.
 * Copyright (C)2018 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "nlutils.h"

struct hex_test {
	char *desc;
	char *data;
	ssize_t len;
	char *hex;
	char *only_hex;
};

/*
 * Reference hex may be generated by:
 * echo -n [data] | hexdump -C | grep '|' | cut -d ' ' -f 3-19 | tr -d ' ' | sed -e 's/^\(.*\)$/\t\t\t"\1"/'
 */
struct hex_test hextests[] = {
	{
		.desc = "Empty string",
		.data = "",
		.len = 0,
		.hex = "",
		.only_hex = "",
	},
	{
		.desc = "Some printable characters",
		.data = "_\"'%*!)#(@*#~_1902835748zZBfjDIELk.,XXOUTPQlS./?",
		.len = 48,
		.hex =	"5f2227252a21292328402a237e5f3139"
			"30323833353734387a5a42666a444945"
			"4c6b2e2c58584f555450516c532e2f3f",
		.only_hex = "5f2227252a21292328402a237e5f3139"
                            "30323833353734387a5a42666a444945"
                            "4c6b2e2c58584f555450516c532e2f3f",
	},
	{
		.desc = "Extra data on the end (after length)",
		.data = "_EXTRA",
		.len = 1,
		.hex = "5f",
		.only_hex = "5f",
	},
	{
		.desc = "Mixed-case hex digits",
		.data = "KLMNOklmno",
		.len = 10,
		.hex = "4b4C4d4E4f6B6C6D6e6F",
		.only_hex = "4b4c4d4e4f6b6c6d6e6f",
	},
	{
		.desc = "Some non-printable characters",
		.data = (char[]){ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 0xff, 0xfe, 0x7f, 0x80 },
		.len = 16,
		.hex = "000102030405060708090a00fffe7f80",
		.only_hex = "000102030405060708090a00fffe7f80",
	},
	{
		.desc = "Extra text after hex",
		.data = "include_null",
		.len = 13,
		.hex = "696e636c7564655f6e756c6c00_ff90cc202122_not_hex_data",
		.only_hex = "696e636c7564655f6e756c6c00ff90cc202122edaa",
	},
	{
		.desc = "Mixed non-hex characters",
		.data = "A",
		.len = 1,
		.hex = "41G_392048dsaiofjcx#$)(*FDOIJvoias0398",
		.only_hex = "41392048dafcfda0398",
	},
};

int do_hex_test(struct hex_test *test)
{
	uint8_t *out_data = NULL;
	char *out_hex = NULL;
	char *hex_copy = NULL;
	ssize_t out_len;

	out_data = malloc(test->len);
	if(out_data == NULL) {
		ERRNO_OUT("Error allocating space for hex test data.\n");
		goto error;
	}

	out_hex = malloc(test->len * 2 + 1);
	if(out_hex == NULL) {
		ERRNO_OUT("Error allocating space for hex test digits.\n");
		goto error;
	}

	DEBUG_OUT("Testing nl_to_hex() on data of length %zu (%s).\n", test->len, test->desc);
	nl_to_hex((uint8_t *)test->data, test->len, out_hex);
	if(strncasecmp(test->hex, out_hex, test->len * 2)) {
		ERROR_OUT("Hexadecimal output mismatch from nl_to_hex().  Got '%s', expected '%s'.\n",
				out_hex, test->hex);
		goto error;
	}

	DEBUG_OUT("Testing nl_from_hex() on '%s' (%s).\n", test->hex, test->desc);
	out_len = nl_from_hex(test->hex, out_data);
	if(out_len != test->len) {
		ERROR_OUT("Length mismatch from nl_from_hex() for %s (%s).  Got %zd, expected %zu.\n",
				test->hex, test->desc,
				out_len, test->len);
		goto error;
	}
	if(memcmp(out_data, test->data, test->len)) {
		ERROR_OUT("Data mismatch from nl_from_hex() for %s (%s).\n", test->hex, test->desc);
		goto error;
	}

	DEBUG_OUT("Testing nl_keep_only_hex() on '%s' (%s).\n", test->hex, test->desc);
	hex_copy = strdup(test->hex);
	if(hex_copy == NULL) {
		ERRNO_OUT("Error copying test hex string");
		goto error;
	}
	nl_keep_only_hex(hex_copy);
	if(strcmp(hex_copy, test->only_hex)) {
		ERROR_OUT("Data mismatch from nl_keep_only_hex() for %s (%s).\n", test->hex, test->desc);
		ERROR_OUT("Got '%s', expected '%s'.\n", hex_copy, test->only_hex);
		goto error;
	}

	free(hex_copy);
	free(out_hex);
	free(out_data);
	return 0;

error:
	free(hex_copy);
	free(out_hex);
	free(out_data);
	return -1;
}

struct strcommon_test {
	char *a;
	char *b;
	size_t l;
};

struct strcommon_test strcommon_tests[] = {
	{
		.a = "String One",
		.b = "Nothing in Common",
		.l = 0,
	},
	{
		.a = "Empty B",
		.b = "",
		.l = 0,
	},
	{
		.a = "",
		.b = "Empty A",
		.l = 0,
	},
	{
		.a = "",
		.b = "",
		.l = 0,
	},
	{
		.a = "Partial Shared Prefix",
		.b = "Partial Match",
		.l = 8,
	},
	{
		.a = "Total Match",
		.b = "Total Match",
		.l = 11,
	},
	{
		.a = "_",
		.b = "_",
		.l = 1,
	},
	{
		.a = "!",
		.b = "@",
		.l = 0,
	},
	{
		.a = "Match with different length",
		.b = "Match",
		.l = 5,
	},
	{
		.a = "Opposite",
		.b = "Opposite direction, but as above",
		.l = 8,
	},
};

int do_strcommon_test(struct strcommon_test *test)
{
	size_t result;

	result = nl_strcommon(test->a, test->b);
	if(result != test->l) {
		ERROR_OUT("nl_strcommon() test of '%s' and '%s' failed.  Got %zu, expected %zu.\n",
				test->a, test->b, result, test->l);
		return -1;
	}

	return 0;
}


/*
 * Describes a line-splitting test with expected lines.
 */
struct line_split_test {
	char *desc; // Description of the test
	struct nl_raw_data data; // Data to split for the test

	// TODO: Add a delimiters member if nl_split_lines() is made generic

	int offset; // current expected line (used by the test function)

	int count; // total expected lines
	struct nl_raw_data *lines; // list of expected lines
};

// Evaluates str multiple times.
#define CONST_STRING_AS_DATA(str) {.size = sizeof(str) - 1, .data = (str)}
#define CONST_STRING_AS_DATA_NUL(str) {.size = sizeof(str), .data = (str)}

static struct line_split_test split_line_tests[] = {
	{
		.desc = "Null data",
		.data = {
			.size = 0,
			.data = NULL,
		},
		.count = 0,
	},
	{
		.desc = "Empty data",
		.data = {
			.size = 0,
			.data = "",
		},
		.count = 0,
	},
	{
		.desc = "Non-zero-terminated empty data",
		.data = {
			.size = 0,
			.data = (char[]){ ' ' },
		},
		.count = 0,
	},
	{
		.desc = "Single character, no line terminators",
		.data = {
			.size = 1,
			.data = " ",
		},
		.count = 1,
		.lines = (struct nl_raw_data[]){
			{ .size = 1, .data = (char[]){' '} },
		},
	},
	{
		.desc = "Single non-0-terminated character, no lines",
		.data = {
			.size = 1,
			.data = (char[]){ '!' },
		},
		.count = 1,
		.lines = (struct nl_raw_data[]){
			{ .size = 1, .data = "!" },
		},
	},
	{
		.desc = "A longer line with no terminators",
		.data = {
			.size = 11,
			.data = "Hello World",
		},
		.count = 1,
		.lines = (struct nl_raw_data[]) {
			{ .size = 11, .data = "Hello World" },
		},
	},
	{
		.desc = "Single CR (\\r)",
		.data = {
			.size = 1,
			.data = (char[]){ '\r' },
		},
		.count = 1,
		.lines = (struct nl_raw_data[]) {
			{ .size = 0, .data = "" },
		},
	},
	{
		.desc = "Single LF (\\n)",
		.data = {
			.size = 1,
			.data = (char[]){ '\n' },
		},
		.count = 1,
		.lines = (struct nl_raw_data[]) {
			{ .size = 0, .data = "" },
		},
	},
	{
		.desc = "CRLF (\\r\\n)",
		.data = {
			.size = 2,
			.data = "\r\n",
		},
		.count = 1,
		.lines = (struct nl_raw_data[]) {
			{ .size = 0, .data = "" },
		},
	},
	{
		.desc = "Mixed line endings, all empty lines",
		.data = {
			.size = 9,
			.data = "\r\r\n\n\n\r\r\n\r",
		},
		.count = 7,
		.lines = (struct nl_raw_data[]) {
			{ .size = 0, .data = "" },
			{ .size = 0, .data = "" },
			{ .size = 0, .data = "" },
			{ .size = 0, .data = "" },
			{ .size = 0, .data = "" },
			{ .size = 0, .data = "" },
			{ .size = 0, .data = "" },
		},
	},
	{
		.desc = "One line, one terminator",
		.data = {
			.size = 16,
			.data = "This is a test\r\n",
		},
		.count = 1,
		.lines = (struct nl_raw_data[]) {
			{ .size = 14, .data = "This is a test" },
		},
	},
	{
		.desc = "Terminator at start",
		.data = {
			.size = 6,
			.data = "\r\ntest",
		},
		.count = 2,
		.lines = (struct nl_raw_data[]) {
			{ .size = 0, .data = "" },
			{ .size = 4, .data = "test" },
		},
	},
	{
		.desc = "Two lines, one terminator",
		.data = {
			.size = 11,
			.data = "Hello\nWorld",
		},
		.count = 2,
		.lines = (struct nl_raw_data[]) {
			{ .size = 5, .data = "Hello" },
			{ .size = 5, .data = "World" },
		},
	},
	{
		.desc = "Two lines, two terminators",
		.data = {
			.size = 12,
			.data = "Hello\nWorld\n",
		},
		.count = 2,
		.lines = (struct nl_raw_data[]) {
			{ .size = 5, .data = "Hello" },
			{ .size = 5, .data = "World" },
		},
	},
	{
		.desc = "Mixed terminators with lines",
		.data = {
			.size = 46,
			.data = "\rThere once \nwas a string\rwith\n\rmany \r\nlines.\n",
		},
		.count = 7,
		.lines = (struct nl_raw_data[]) {
			{ .size = 0, .data = "" },
			{ .size = 11, .data = "There once " },
			{ .size = 12, .data = "was a string" },
			{ .size = 4, .data = "with" },
			{ .size = 0, .data = "" },
			{ .size = 5, .data = "many " },
			{ .size = 6, .data = "lines." },
		},
	},
	{
		.desc = "HTTP response",
		.data = CONST_STRING_AS_DATA(
				"HTTP/1.1 200 OK\r\n"
				"Date: Sun, 21 Sep 2014 22:33:05 GMT\r\n"
				"Server: Apache/2.2.16 (Debian) PHP/5.3.3-7+squeeze17 with Suhosin-Patch\r\n"
				"Last-Modified: Sat, 29 Mar 2014 00:36:43 GMT\r\n"
				"ETag: \"c687-1fd2-4f5b4032c5cc0\"\r\n"
				"Accept-Ranges: bytes\r\n"
				"Content-Length: 8146\r\n"
				"Vary: Accept-Encoding\r\n"
				"Content-Type: text/html\r\n"
				"\r\n"
				),
		.count = 10,
		.lines = (struct nl_raw_data[]) {
			CONST_STRING_AS_DATA("HTTP/1.1 200 OK"),
			CONST_STRING_AS_DATA("Date: Sun, 21 Sep 2014 22:33:05 GMT"),
			CONST_STRING_AS_DATA("Server: Apache/2.2.16 (Debian) PHP/5.3.3-7+squeeze17 with Suhosin-Patch"),
			CONST_STRING_AS_DATA("Last-Modified: Sat, 29 Mar 2014 00:36:43 GMT"),
			CONST_STRING_AS_DATA("ETag: \"c687-1fd2-4f5b4032c5cc0\""),
			CONST_STRING_AS_DATA("Accept-Ranges: bytes"),
			CONST_STRING_AS_DATA("Content-Length: 8146"),
			CONST_STRING_AS_DATA("Vary: Accept-Encoding"),
			CONST_STRING_AS_DATA("Content-Type: text/html"),
			CONST_STRING_AS_DATA(""),
		},
	},
	{
		.desc = "Incorrectly NUL-terminated data",
		.data = CONST_STRING_AS_DATA_NUL("Test\n"),
		.count = 2,
		.lines = (struct nl_raw_data[]) {
			CONST_STRING_AS_DATA("Test"),
			{ .size = 1, .data = (char[]){ 0 } },
		},
	},
	{
		.desc = "Data containing NUL bytes",
		.data = {
			.size = 11,
			.data = (char[]){ 'H', 'i', 0, 'b', 'y', 'e', '\r', '\n', 0, '\n', '\n' },
		},
		.count = 3,
		.lines = (struct nl_raw_data[]) {
			{ .size = 6, .data = (char[]){ 'H', 'i', 0, 'b', 'y', 'e' } },
			{ .size = 1, .data = (char[]){ 0 } },
			{ .size = 0, .data = "" },
		},
	},
};

static int line_split_test_cb(struct nl_raw_data line, void *cb_data)
{
	struct line_split_test *test = cb_data;
	int idx = test->offset;
	int fail = 0;

	DEBUG_OUT_EX("\tReceived line %d of %d for '%s'\n", idx + 1, test->count, test->desc);

	if(idx >= test->count) {
		ERROR_OUT("Received unexpected line %d for '%s'\n",
				idx + 1, test->desc);
		exit(1);
	}

	if(line.data == NULL) {
		ERROR_OUT("BUG: Got a NULL pointer for the incoming line's data on %s.\n", test->desc);
		exit(1);
	} else if(line.size != test->lines[idx].size) {
		ERROR_OUT("Size mismatch (got %zu, expected %zu) on %s.\n",
				line.size, test->lines[idx].size, test->desc);
		fail = 1;
	} else if(memcmp(line.data, test->lines[idx].data, line.size)) {
		ERROR_OUT("Data mismatch on %s.\n", test->desc);
		fail = 1;
	}

	if(fail) {
		ERROR_OUT("Incoming data: ");
		fwrite(line.data, line.size, 1, stderr);
		ERROR_OUT_EX("\n");
		ERROR_OUT("Expected data: ");
		fwrite(test->lines[idx].data, test->lines[idx].size, 1, stderr);
		ERROR_OUT_EX("\n");
		exit(1);
	}

	test->offset++;

	return 0;
}

static int line_split_break_cb(struct nl_raw_data line, void *cb_data)
{
	struct line_split_test *test = cb_data;

	if(line.data == NULL) {
		ERROR_OUT("BUG: Got a NULL pointer for line's data on interrupted %s.\n", test->desc);
		exit(1);
	}

	return -1;
}

static void do_line_split_test(struct line_split_test *test)
{
	int ret;

	DEBUG_OUT("Checking: %s\n", test->desc);

	test->offset = 0;
	ret = nl_split_lines(test->data, line_split_test_cb, test);

	if(test->offset != test->count) {
		ERROR_OUT("nl_split_lines test %s produced %d lines, expected %d.\n",
				test->desc, test->offset, test->count);
		exit(1);
	}

	if(ret != test->count) {
		ERROR_OUT("Return value from nl_split_lines for %s is %d, expected %d.\n",
				test->desc, ret, test->count);
		exit(1);
	}

	DEBUG_OUT("Testing early interruption with %s.\n", test->desc);
	ret = nl_split_lines(test->data, line_split_break_cb, test);

	if(ret != MIN_NUM(test->count, 1)) {
		ERROR_OUT("Return value from interrupted split is %d, expected %d.\n",
				ret, MIN_NUM(test->count, 1));
		exit(1);
	}
}

int main()
{
	size_t i;

	INFO_OUT("Testing nl_to_hex() and nl_from_hex().\n");
	for(i = 0; i < ARRAY_SIZE(hextests); i++) {
		if(do_hex_test(&hextests[i])) {
			ERROR_OUT("Tests of nl_to_hex() and nl_from_hex() failed.\n");
			return -1;
		}
	}

	INFO_OUT("Testing nl_strcommon().\n");
	for(i = 0; i < ARRAY_SIZE(strcommon_tests); i++) {
		if(do_strcommon_test(&strcommon_tests[i])) {
			ERROR_OUT("Tests of nl_strcommon() failed.\n");
			return -1;
		}
	}

	INFO_OUT("Testing nl_split_lines().\n");
	for(i = 0; i < ARRAY_SIZE(split_line_tests); i++) {
		do_line_split_test(&split_line_tests[i]);
	}

	// TODO: More string function tests (count, dup, end, start, etc. functions)

	INFO_OUT("String tests succeeded.\n");

	return 0;
}
