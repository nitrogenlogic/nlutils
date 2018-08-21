/*
 * Tests for network-related utility functions.
 * Copyright (C)2011 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>

#include "nlutils.h"

struct mac_test {
	char *mac_in; // Input MAC address
	char *mac_out; // Parsed MAC address
	int ret; // Return value from nl_parse_mac()
};

struct mac_test mac_tests[] = {
	{
		.mac_in = NULL,
		.mac_out = NULL,
		.ret = -EFAULT,
	},
	{
		.mac_in = "",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "00:00:00:00:00:00",
		.mac_out = "000000000000",
		.ret = 0,
	},
	{
		.mac_in = "000000000000",
		.mac_out = "000000000000",
		.ret = 0,
	},
	{
		.mac_in = "ffffffffffff",
		.mac_out = "ffffffffffff",
		.ret = 0,
	},
	{
		.mac_in = "a0b1c2d3e4f5",
		.mac_out = "a0b1c2d3e4f5",
		.ret = 0,
	},
	{
		.mac_in = "00:00:00:00:00:00:",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "00:0000000000",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "000000:000000",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "000:000000000",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "f0f0f0f0f0f0f",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "00-11-22-33-44-5a",
		.mac_out = "00112233445a",
		.ret = 0,
	},
	{
		.mac_in = "00-11:22:33:44-55",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "F0-f0-F0-ab-Cd-eF",
		.mac_out = "f0f0f0abcdef",
		.ret = 0,
	},
	{
		.mac_in = "f3:f3-f3-f3-f3-3F",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "0:0:0:0:0:0:0:0:0:0:0:0",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "FE:dC:Ba:98:765F",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "FE:dC:Ba:98:76-5F",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "FE:dC:Ba:98:76:5F",
		.mac_out = "fedcba98765f",
		.ret = 0,
	},
	{
		.mac_in = "00:00:00:00:00:0g",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "00.00.00.00.00.00",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "00_00_00_00_00_00",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "_z",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "127.0.0.1",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "c",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "cc",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "cc:",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "cc:cc:cc:cc:cc:",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "cc:cc:cc:cc:cc:c",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
	{
		.mac_in = "FeDcBaAbCdEf",
		.mac_out = "fedcbaabcdef",
		.ret = 0,
	},
	{
		.mac_in = "fE-dC-bA-Ab-Cd-Ef",
		.mac_out = "fedcbaabcdef",
		.ret = 0,
	},
	{
		.mac_in = "An oversized string that most certainly will not work.",
		.mac_out = NULL,
		.ret = -EINVAL,
	},
};

// Tests nl_parse_mac() using the given mac test.
int test_mac(struct mac_test *data, char separator)
{
	char outbuf[18] = "unmodified";
	char sepbuf[18] = "";
	int ret;
	int i, j;

	DEBUG_OUT("Testing nl_parse_mac() with '%s'.\n", GUARD_NULL(data->mac_in));
	if(separator) {
		DEBUG_OUT("\tTesting with separator: %c.\n", separator);
	}

	if(separator && data->mac_out) {
		for(i = 0, j = 0; i < 18; i += 3, j += 2) {
			sepbuf[i] = data->mac_out[j];
			sepbuf[i + 1] = data->mac_out[j + 1];
			if(i < 15) {
				sepbuf[i + 2] = separator;
			} else {
				sepbuf[i + 2] = 0;
			}
		}
	}

	// Test with mac_out
	ret = nl_parse_mac(data->mac_in, outbuf, separator);
	if(ret != data->ret) {
		ERROR_OUT("Invalid return for '%s': expected %d (%s), got %d (%s).\n",
				GUARD_NULL(data->mac_in),
				data->ret, strerror(-data->ret),
				ret, strerror(-ret));
		return -1;
	}
	if(ret == 0 && strcmp(outbuf, separator ? sepbuf : data->mac_out)) {
		ERROR_OUT("Non-matching output MAC for '%s': expected '%s', got '%s'.\n",
				GUARD_NULL(data->mac_in), separator ? sepbuf : data->mac_out, outbuf);
		return -1;
	}
	if(ret && strcmp(outbuf, "unmodified")) {
		ERROR_OUT("mac_out was modified with an invalid MAC address: expected '%s', got '%s'.\n",
				"unmodified", outbuf);
		return -1;
	}

	// Test without mac_out
	ret = nl_parse_mac(data->mac_in, NULL, separator);
	if(ret != data->ret) {
		ERROR_OUT("Invalid return for '%s' (null mac_out): expected %d (%s), got %d (%s).\n",
				GUARD_NULL(data->mac_in),
				data->ret, strerror(-data->ret),
				ret, strerror(-ret));
		return -1;
	}

	return 0;
}

int main()
{
	size_t i;

	for(i = 0; i < ARRAY_SIZE(mac_tests); i++) {
		if(test_mac(&mac_tests[i], 0) ||
				test_mac(&mac_tests[i], ':') ||
				test_mac(&mac_tests[i], '-') ||
				test_mac(&mac_tests[i], '.') ||
				test_mac(&mac_tests[i], '3') ||
				test_mac(&mac_tests[i], 'z')) {
			ERROR_OUT("nl_parse_mac() tests failed.\n");
			return -1;
		}
	}
	
	return 0;
}
