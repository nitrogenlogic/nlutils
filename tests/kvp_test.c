/*
 * Tests for key-value pair parsing functions.
 * Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>

#include "nlutils.h"

/*
 * Describes an expected key-value pair.
 */
struct kvpair {
	char *key; // Key name
	char *strvalue; // String value
	struct nl_variant value; // Parsed variant value
};

/*
 * Describes a key-value pair parsing test with expected pairs.
 */
struct kvp_test {
	char *desc; // description of this test
	char *data; // key-value pair line to parse

	int offset; // current expected pair
	int count; // total expected pairs
	struct kvpair *pairs; // list of expected pairs
};

struct kvp_test kvptests[] = {
	{
		.desc = "Null string",
		.data = NULL,
		.count = 0,
	},
	{
		.desc = "Empty string",
		.data = "",
		.count = 0,
	},
	{
		.desc = "Empty pairs",
		.data = "a=\"\" \"\"=b \"\"=\"\"",
		.count = 3,
		.pairs = (struct kvpair[]){
			{ .key = "a", .strvalue = "", .value = { STRING, { .string = "" } } },
			{ .key = "", .strvalue = "b", .value = { STRING, { .string = "b" } } },
			{ .key = "", .strvalue = "", .value = { STRING, { .string = "" } } },
		},
	},
	{
		.desc = "Single unquoted string pair",
		.data = "a=b",
		.count = 1,
		.pairs = (struct kvpair[]){
			{ .key = "a", .strvalue = "b", .value = { STRING, { .string = "b" } } },
		},
	},
	{
		.desc = "Single unquoted integer pair",
		.data = "a=0",
		.count = 1,
		.pairs = (struct kvpair[]){
			{ .key = "a", .strvalue = "0", .value = { INTEGER, { .integer = 0 } } },
		},
	},
	{
		.desc = "Single quoted integer as integer",
		.data = "\"a\"=0",
		.count = 1,
		.pairs = (struct kvpair[]){
			{ .key = "a", .strvalue = "0", .value = { INTEGER, { .integer = 0 } } },
		},
	},
	{
		.desc = "Quotation variations",
		.data = "\"a\"=b a=\"b\" \"a\"=\"b\"",
		.count = 3,
		.pairs = (struct kvpair[]){
			{ .key = "a", .strvalue = "b", .value = { STRING, { .string = "b" } } },
			{ .key = "a", .strvalue = "b", .value = { STRING, { .string = "b" } } },
			{ .key = "a", .strvalue = "b", .value = { STRING, { .string = "b" } } },
		},
	},
	{
		.desc = "Single quoted integer as string",
		.data = "\"a\"=\"0\"",
		.count = 1,
		.pairs = (struct kvpair[]){
			{ .key = "a", .strvalue = "0", .value = { STRING, { .string = "0" } } },
		},
	},
	{
		.desc = "Whitespace outside of pairs, hexadecimal integer",
		.data = "  \r\n\t\va=b      \t  c=0x0dd  \n\r\n",
		.count = 2,
		.pairs = (struct kvpair[]){
			{ .key = "a", .strvalue = "b", .value = { STRING, { .string = "b" } } },
			{ .key = "c", .strvalue = "0x0dd", .value = { INTEGER, { .integer = 0x0dd } } },
		},
	},
	{
		.desc = "Whitespace inside pairs",
		.data = "\" a \"=\"\n\tb\t\n\"",
		.count = 1,
		.pairs = (struct kvpair[]){
			{ .key = " a ", .strvalue = "\n\tb\t\n", .value = { STRING, { .string = "\n\tb\t\n" } } },
		},
	},
	{
		.desc = "Boolean values",
		.data = "true=true false=false",
		.count = 2,
		.pairs = (struct kvpair[]) {
			{ .key = "true", .strvalue = "true", .value = { INTEGER, { .integer = 1 } } },
			{ .key = "false", .strvalue = "false", .value = { INTEGER, { .integer = 0 } } },
		},
	},
	{
		.desc = "Quoted booleans, swapped boolean names as keys",
		.data = "\"true\"=false \"false\"=true truestr=\"true\" falsestr=\"false\"",
		.count = 4,
		.pairs = (struct kvpair[]) {
			{ .key = "true", .strvalue = "false", .value = { INTEGER, { .integer = 0 } } },
			{ .key = "false", .strvalue = "true", .value = { INTEGER, { .integer = 1 } } },
			{ .key = "truestr", .strvalue = "true", .value = { STRING, { .string = "true" } } },
			{ .key = "falsestr", .strvalue = "false", .value = { STRING, { .string = "false" } } },
		},
	},
	{
		.desc = "Escape sequences",
		.data = "\"\\\"a\\\"\"=\"A\\x20\\\\B\\n\" b\\\\=\\x20",
		.count = 2,
		.pairs = (struct kvpair[]){
			{ .key = "\"a\"", .strvalue = "A \\B\n", .value = { STRING, { .string = "A \\B\n" } } },
			{ .key = "b\\\\", .strvalue = "\\x20", .value = { STRING, { .string = "\\x20" } } },
		},
	},
	{
		.desc = "Unusual key and value",
		.data = ",=! x\"=y\"",
		.count = 2,
		.pairs = (struct kvpair[]) {
			{ .key = ",", .strvalue = "!", .value = { STRING, { .string = "!" } } },
			{ .key = "x\"", .strvalue = "y\"", .value = { STRING, { .string = "y\"" } } },
		},
	},
	{
		.desc = "Integers",
		.data = "a=+0 b=+1 c=-1 d=2147483647 e=-2147483648 f=0x7fffffff g=0xFFffFfFf",
		.count = 7,
		.pairs = (struct kvpair[]) {
			{ .key = "a", .strvalue = "+0", .value = { INTEGER, { .integer = 0 } } },
			{ .key = "b", .strvalue = "+1", .value = { INTEGER, { .integer = 1 } } },
			{ .key = "c", .strvalue = "-1", .value = { INTEGER, { .integer = -1 } } },
			{ .key = "d", .strvalue = "2147483647", .value = { INTEGER, { .integer = 2147483647 } } },
			{ .key = "e", .strvalue = "-2147483648", .value = { INTEGER, { .integer = -2147483648 } } },
			{ .key = "f", .strvalue = "0x7fffffff", .value = { INTEGER, { .integer = 0x7fffffff } } },
			{ .key = "g", .strvalue = "0xFFffFfFf", .value = { INTEGER, { .integer = 0xffffffff } } },
		},
	},
	{
		.desc = "Floating point",
		.data = "a=1. b=1E5 c=-1.0 d=+1.0 e=+1e5 f=-1e5 g=1.024e+4 h=125e-0 i=125e-3 j=\"1.0\" k=.5",
		.count = 11,
		.pairs = (struct kvpair[]) {
			{ .key = "a", .strvalue = "1.", .value = { FLOAT, { .floating = 1.0 } } },
			{ .key = "b", .strvalue = "1E5", .value = { FLOAT, { .floating = 1e5 } } },
			{ .key = "c", .strvalue = "-1.0", .value = { FLOAT, { .floating = -1.0 } } },
			{ .key = "d", .strvalue = "+1.0", .value = { FLOAT, { .floating = +1.0 } } },
			{ .key = "e", .strvalue = "+1e5", .value = { FLOAT, { .floating = +1e5 } } },
			{ .key = "f", .strvalue = "-1e5", .value = { FLOAT, { .floating = -1e5 } } },
			{ .key = "g", .strvalue = "1.024e+4", .value = { FLOAT, { .floating = 1.024e+4 } } },
			{ .key = "h", .strvalue = "125e-0", .value = { FLOAT, { .floating = 125e-0 } } },
			{ .key = "i", .strvalue = "125e-3", .value = { FLOAT, { .floating = 125e-3 } } },
			{ .key = "j", .strvalue = "1.0", .value = { STRING, { .string = "1.0" } } },
			{ .key = "k", .strvalue = ".5", .value = { FLOAT, { .floating = 0.5 } } },
		},
	},
	{
		.desc = "Example from wrapper method comment",
		.data = "key1=val1 key2=0",
		.count = 2,
		.pairs = (struct kvpair[]){
			{ .key = "key1", .strvalue = "val1", .value = { STRING, { .string = "val1" } } },
			{ .key = "key2", .strvalue = "0", .value = { INTEGER, { .integer = 0 } } },
		},
	},
	{
		.desc = "Example from parse method comment",
		.data = "  a=b \"c\"=d e=f=g \"e\"=\"f=g\" \"g \\\"h i j\"=\" k\\\"l\\\"mn \"",
		.count = 5,
		.pairs = (struct kvpair[]){
			{ .key = "a", .strvalue = "b", .value = { STRING, { .string = "b" } } },
			{ .key = "c", .strvalue = "d", .value = { STRING, { .string = "d" } } },
			{ .key = "e", .strvalue = "f=g", .value = { STRING, { .string = "f=g" } } },
			{ .key = "e", .strvalue = "f=g", .value = { STRING, { .string = "f=g" } } },
			{ .key = "g \"h i j", .strvalue = " k\"l\"mn ", .value = { STRING, { .string = " k\"l\"mn " } } },
		},
	},
	{
		.desc = "Garbage around pairs",
		.data = "z\\\"coij zx3289 \"RW\\tEj\" q= ej89a34r =dzsa 30r5ui a;f=fasd==e q=\"r\"z 3=4 ##()*$\\",
		.count = 3,
		.pairs = (struct kvpair[]){
			{ .key = "a;f", .strvalue = "fasd==e", .value = { STRING, { .string = "fasd==e" } } },
			{ .key = "q", .strvalue = "r", .value = { STRING, { .string = "r" } } },
			{ .key = "3", .strvalue = "4", .value = { INTEGER, { .integer = 4 } } },
		},
	},
	{
		.desc = "Equal sign spacing tests",
		.data = "a=b c = d e= f g =h i=\" j \" \"a=\"b \"c=\"=\"=d\" e\"=f\"",
		.count = 4,
		.pairs = (struct kvpair[]){
			{ .key = "a", .strvalue = "b", .value = { STRING, { .string = "b" } } },
			{ .key = "i", .strvalue = " j ", .value = { STRING, { .string = " j " } } },
			{ .key = "c=", .strvalue = "=d", .value = { STRING, { .string = "=d" } } },
			{ .key = "e\"", .strvalue = "f\"", .value = { STRING, { .string = "f\"" } } },
		},
	},
	{
		.desc = "Unterminated string",
		.data = "\"a\"=\"b",
		.count = 1,
		.pairs = (struct kvpair[]){
			{ .key = "a", .strvalue = "b", .value = { STRING, { .string = "b" } } },
		},
	},
	{
		.desc = "Invalid data mixed with pairs",
		.data = "XYZ - a=1 b=\"2\" bogus - q=9.0 END",
		.count = 3,
		.pairs = (struct kvpair[]){
			{ .key = "a", .strvalue = "1", .value = { INTEGER, { .integer = 1 } } },
			{ .key = "b", .strvalue = "2", .value = { STRING, { .string = "2" } } },
			{ .key = "q", .strvalue = "9.0", .value = { FLOAT, { .floating = 9.0 } } },
		},
	},
	{
		.desc = "Invalid floating point values",
		.data = "a=-3E5.0 a=+3e+ a=+3e a=+e a=-e3 a=-3.e1 a=-3.e.1 a=1.e a=1.e0 a=. a=+ a=-",
		.count = 12,
		.pairs = (struct kvpair[]){
			{ .key = "a", .strvalue = "-3E5.0", .value = { STRING, { .string = "-3E5.0" } } },
			{ .key = "a", .strvalue = "+3e+", .value = { STRING, { .string = "+3e+" } } },
			{ .key = "a", .strvalue = "+3e", .value = { STRING, { .string = "+3e" } } },
			{ .key = "a", .strvalue = "+e", .value = { STRING, { .string = "+e" } } },
			{ .key = "a", .strvalue = "-e3", .value = { STRING, { .string = "-e3" } } },
			{ .key = "a", .strvalue = "-3.e1", .value = { STRING, { .string = "-3.e1" } } },
			{ .key = "a", .strvalue = "-3.e.1", .value = { STRING, { .string = "-3.e.1" } } },
			{ .key = "a", .strvalue = "1.e", .value = { STRING, { .string = "1.e" } } },
			{ .key = "a", .strvalue = "1.e0", .value = { STRING, { .string = "1.e0" } } },
			{ .key = "a", .strvalue = ".", .value = { STRING, { .string = "." } } },
			{ .key = "a", .strvalue = "+", .value = { STRING, { .string = "+" } } },
			{ .key = "a", .strvalue = "-", .value = { STRING, { .string = "-" } } },
		},
	},
	{
		.desc = "Long realistic data",
		.data = "xmin=-1807 ymin=-398 zmin=3430 xmax=-1564 ymax=-11 zmax=3710 px_xmin=573 px_ymin=241 px_zmin=990 px_xmax=637 px_ymax=309 px_zmax=998 occupied=0 pop=0 maxpop=4352 xc=0 yc=0 zc=0 sa=0 name=\"Name\"",
		.count = 20,
		.pairs = (struct kvpair[]){
			{ .key = "xmin", .strvalue = "-1807", .value = { INTEGER, { .integer = -1807 } } },
			{ .key = "ymin", .strvalue = "-398", .value = { INTEGER, { .integer = -398 } } },
			{ .key = "zmin", .strvalue = "3430", .value = { INTEGER, { .integer = 3430 } } },
			{ .key = "xmax", .strvalue = "-1564", .value = { INTEGER, { .integer = -1564 } } },
			{ .key = "ymax", .strvalue = "-11", .value = { INTEGER, { .integer = -11 } } },
			{ .key = "zmax", .strvalue = "3710", .value = { INTEGER, { .integer = 3710 } } },
			{ .key = "px_xmin", .strvalue = "573", .value = { INTEGER, { .integer = 573 } } },
			{ .key = "px_ymin", .strvalue = "241", .value = { INTEGER, { .integer = 241 } } },
			{ .key = "px_zmin", .strvalue = "990", .value = { INTEGER, { .integer = 990 } } },
			{ .key = "px_xmax", .strvalue = "637", .value = { INTEGER, { .integer = 637 } } },
			{ .key = "px_ymax", .strvalue = "309", .value = { INTEGER, { .integer = 309 } } },
			{ .key = "px_zmax", .strvalue = "998", .value = { INTEGER, { .integer = 998 } } },
			{ .key = "occupied", .strvalue = "0", .value = { INTEGER, { .integer = 0 } } },
			{ .key = "pop", .strvalue = "0", .value = { INTEGER, { .integer = 0 } } },
			{ .key = "maxpop", .strvalue = "4352", .value = { INTEGER, { .integer = 4352 } } },
			{ .key = "xc", .strvalue = "0", .value = { INTEGER, { .integer = 0 } } },
			{ .key = "yc", .strvalue = "0", .value = { INTEGER, { .integer = 0 } } },
			{ .key = "zc", .strvalue = "0", .value = { INTEGER, { .integer = 0 } } },
			{ .key = "sa", .strvalue = "0", .value = { INTEGER, { .integer = 0 } } },
			{ .key = "name", .strvalue = "Name", .value = { STRING, { .string = "Name" } } },
		},
	},
};

static void kvp_test_cb(void *cb_data, char *key, char *strvalue, struct nl_variant value)
{
	struct kvp_test *test = cb_data;
	int idx = test->offset;

	DEBUG_OUT_EX("\tReceived pair %d of %d for '%s'\n", idx + 1, test->count, test->desc);

	if(idx >= test->count) {
		ERROR_OUT("Received unexpected pair %d (%s=%s) for '%s'\n",
				idx + 1, key, strvalue, test->desc);
		exit(1);
	}

	struct kvpair *pair = &test->pairs[idx];

	if(strcmp(key, pair->key)) {
		 ERROR_OUT("Key mismatch (got %s, expected %s) on pair %d (%s=%s) for '%s'\n",
				 key, pair->key, idx, key, strvalue, test->desc);
		 exit(1);
	}

	if(strcmp(strvalue, pair->strvalue)) {
		 ERROR_OUT("Value string mismatch (got %s, expected %s) on pair %d (%s=%s) for '%s'\n",
				 strvalue, pair->strvalue, idx, key, strvalue, test->desc);
		 exit(1);
	}

	if(nl_compare_variants(value, pair->value)) {
		ERROR_OUT("Variant value mismatch (got %s ", NL_VARTYPE_NAME(value.type));
		nl_fprint_variant(stderr, value);
		ERROR_OUT_EX(", expected %s ", NL_VARTYPE_NAME(pair->value.type));
		nl_fprint_variant(stderr, pair->value);
		ERROR_OUT_EX(") on pair %d (%s=%s) for '%s'\n",
				idx, key, strvalue, test->desc);
		exit(1);
	}

	test->offset++;
}

static void do_kvp_test(struct kvp_test *test)
{
	printf("Checking: %s\n", test->desc);

	test->offset = 0;
	nl_parse_kvp_variant(test->data, kvp_test_cb, test);

	if(test->offset != test->count) {
		ERROR_OUT("Test %s produced %d pairs, expected %d.\n",
				test->desc, test->offset, test->count);
		exit(1);
	}

	// TODO: test nl_parse_kvp_hash()
}

int main()
{
	printf("Making sure a NULL callback doesn't crash.\n");
	nl_parse_kvp("", NULL, NULL);

	for(size_t i = 0; i < ARRAY_SIZE(kvptests); i++) {
		do_kvp_test(&kvptests[i]);
	}

	return 0;
}
