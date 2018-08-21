/*
 * Tests associative array functions.
 * Copyright (C)2014 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "nlutils.h"

// Keys and values will be concatenated together during iteration tests.  The
// concatenated string tests iteration order, the count verifies that iteration
// stops when expected, based on max_count.
struct iterator_params {
	// Concatenated keys and values (output)
	char concatenated[512];

	// Number of iterations thus far (output)
	int count;

	// Number of rounds to iterate before returning nonzero from callback,
	// 0 for unlimited (input)
	int max_count;
};

// Test data before and after iterating the 4-entry cloned hash below
struct iterator_test {
	struct iterator_params before;
	struct iterator_params after;
};

static struct iterator_test tests[] = {
	{
		// Full iteration
		.before = {
			.concatenated = "",
			.count = 0,
			.max_count = 0,
		},
		.after = {
			.concatenated = "Nitrogen=Logic\none=two\nthree=four\nfive=six\n",
			.count = 4,
			.max_count = 0,
		},
	},
	{
		// Limited iteration (1)
		.before = {
			.concatenated = "",
			.count = 0,
			.max_count = 1,
		},
		.after = {
			.concatenated = "Nitrogen=Logic\n",
			.count = 1,
			.max_count = 1,
		},
	},
	{
		// Limited iteration (2)
		.before = {
			.concatenated = "",
			.count = 0,
			.max_count = 2,
		},
		.after = {
			.concatenated = "Nitrogen=Logic\none=two\n",
			.count = 2,
			.max_count = 2,
		},
	},
	{
		// Limited iteration (3)
		.before = {
			.concatenated = "",
			.count = 0,
			.max_count = 3,
		},
		.after = {
			.concatenated = "Nitrogen=Logic\none=two\nthree=four\n",
			.count = 3,
			.max_count = 3,
		},
	},
	{
		// Limited iteration (4)
		.before = {
			.concatenated = "",
			.count = 0,
			.max_count = 4,
		},
		.after = {
			.concatenated = "Nitrogen=Logic\none=two\nthree=four\nfive=six\n",
			.count = 4,
			.max_count = 4,
		},
	},
};

// nl_hash_iterate() callback
static int test_callback(void *cb_data, char *key, char *value)
{
	struct iterator_params *test = cb_data;

	// Kind of O(n^2) calling strlen a bunch, but the strings are short so
	// this is easier
	snprintf(test->concatenated + strlen(test->concatenated), sizeof(test->concatenated) - strlen(test->concatenated),
			"%s=%s\n", key, value);

	test->count++;

	if(test->max_count != 0 && test->count == test->max_count) {
		return 1;
	}

	return 0;
}

static int run_iterator_tests(struct nl_hash *hash)
{
	int ret = 0;

	for(size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		struct iterator_params p = tests[i].before;
		nl_hash_iterate(hash, test_callback, &p);

		if(strcmp(p.concatenated, tests[i].after.concatenated)) {
			ERROR_OUT("Concatenated strings don't match on test %zu.  Expected '%s', got '%s'\n",
					i, tests[i].after.concatenated, p.concatenated);
			ret = -1;
		}

		if(p.count != tests[i].after.count) {
			ERROR_OUT("Iteration count doesn't match on test %zu.  Expected %d, got %d\n",
					i, tests[i].after.count, p.count);
			ret = -1;
		}

		if(p.max_count != tests[i].after.max_count) {
			ERROR_OUT("BUG: Iteration limit doesn't match on test %zu.  Expected %d, got %d\n",
					i, tests[i].after.max_count, p.max_count);
			ret = -1;
		}
	}

	return ret;
}

int main(void)
{
	struct nl_hash *hash, *cloned;

	INFO_OUT("Testing associative array/hash table functions.\n");

	hash = nl_hash_create();
	if(hash == NULL) {
		ERROR_OUT("Error creating a hash table.\n");
	}

	nl_ptmf("Test null parameters to nl_hash_set\n");
	if(nl_hash_set(NULL, NULL, NULL) != -1 ||
			nl_hash_set(hash, NULL, NULL) != -1 ||
			nl_hash_set(hash, "test", NULL) != -1) {
		ERROR_OUT("Incorrect result for NULL parameters to nl_hash_set().\n");
		return -1;
	}

	nl_ptmf("Test null parameter to nl_hash_clone\n");
	if(nl_hash_clone(NULL) != NULL) {
		ERROR_OUT("Should not be able to clone a NULL hash\n");
	}

	nl_ptmf("Test get and set\n");
	if(nl_hash_set(hash, "Nitrogen", "Logic")) {
		ERROR_OUT("Error adding a test key to a hash.\n");
		return -1;
	}
	if(strcmp(nl_hash_get(hash, "Nitrogen"), "Logic")) {
		ERROR_OUT("Value returned by nl_hash_get() didn't match.\n");
		return -1;
	}
	if(hash->count != 1) {
		ERROR_OUT("Wrong number of elements in hash table (expected 1, got %zu)\n", hash->count);
		return -1;
	}

	nl_ptmf("Test clone\n");
	if(nl_hash_set(hash, "one", "two") || nl_hash_set(hash, "three", "four")) {
		ERROR_OUT("Error adding more test keys to hash.\n");
		return -1;
	}
	cloned = nl_hash_clone(hash);
	if(cloned == NULL) {
		ERROR_OUT("Cloned hash was NULL\n");
		return -1;
	}
	if(cloned == hash) {
		ERROR_OUT("Cloned hash is the same as the original hash\n");
		return -1;
	}

	if(nl_hash_remove(hash, "one") || nl_hash_set(cloned, "five", "six")) {
		ERROR_OUT("Error modifying original and cloned hash.\n");
		return -1;
	}
	if(hash->count != 2) {
		ERROR_OUT("Original hash has %zu entries after clone, expected 2\n", hash->count);
		return -1;
	}
	if(cloned->count != 4) {
		ERROR_OUT("Cloned hash has %zu entries after clone, expected 4\n", cloned->count);
		return -1;
	}

	nl_ptmf("Test clear\n");
	nl_hash_clear(hash);
	if(hash->count != 0) {
		ERROR_OUT("Hash table should be empty (has %zu elements)\n", hash->count);
		return -1;
	}
	if(cloned->count != 4) {
		ERROR_OUT("Cloned table was altered by clear, but it should not have been\n");
		return -1;
	}

	nl_ptmf("Test iteration\n");
	// Test iterating over cloned hash with 4 entries
	if(run_iterator_tests(cloned)) {
		return -1;
	}

	// Test empty iteration, expect no calls
	struct iterator_params params = { .concatenated = "", .count = 0, .max_count = 0 };
	nl_hash_iterate(hash, test_callback, &params);
	if(params.count != 0) {
		ERROR_OUT("Iterator callback was called for an empty hash\n");
		return -1;
	}

	nl_ptmf("Test clone of empty hash\n");
	nl_hash_destroy(cloned);
	cloned = nl_hash_clone(hash);
	if(cloned->count != 0) {
		ERROR_OUT("Expected cloned empty hash to be, well, empty\n");
		return -1;
	}
	if(nl_hash_set(cloned, "can set", "key in empty clone")) {
		ERROR_OUT("Error setting a key in a cloned empty hash\n");
		return -1;
	}

	nl_hash_destroy(hash);
	nl_hash_destroy(cloned);

	INFO_OUT("\e[32mAssociative array/hash table tests succeeded.\e[0m\n");

	return 0;
}
