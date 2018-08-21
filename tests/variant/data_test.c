/*
 * Tests for functions related to nl_raw_data.
 * Copyright (C)2017 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>

#include "nlutils.h"

int check_struct(const struct nl_raw_data value, const struct nl_raw_data expected)
{
	int ret = 0;

	if(CHECK_NOT_EQUAL(value.size, expected.size)) {
		ERROR_OUT("Expected .size %zu, got .size %zu\n", expected.size, value.size);
		ret = -1;
	}

	if(expected.data == NULL && value.data != NULL) {
		ERROR_OUT("Expected NULL .data, got non-NULL .data\n");
		ret = -1;
	}

	if(expected.data != NULL && value.data == NULL) {
		ERROR_OUT("Expected non-NULL .data, got NULL .data\n");
		ret = -1;
	}

	if(expected.data == value.data && expected.data != NULL) {
		ERROR_OUT("Expected and returned .data are the same pointer; they should differ.\n");
		ret = -1;
	}

	if(value.size > 0 && value.size != SIZE_MAX && value.data == NULL) {
		ERROR_OUT(".size is greater than zero with NULL .data\n");
		ret = -1;
	}

	if(value.size > 0 && value.size >= expected.size && value.data != NULL && expected.data != NULL &&
			memcmp(expected.data, value.data, value.size)) {
		ERROR_OUT("Data returned does not match expected data.\n");

		ERROR_OUT("\tReturned: ");
		nl_display_variant(stderr, NL_DATA_TO_VARIANT(value));
		fprintf(stderr, " - ");
		fwrite(value.data, value.size, 1, stderr);

		fprintf(stderr, "\n\tExpected: ");
		nl_display_variant(stderr, NL_DATA_TO_VARIANT(expected));
		fprintf(stderr, " - ");
		fwrite(expected.data, expected.size, 1, stderr);

		fprintf(stderr, "\n");
	}
	
	return ret;
}

int check_pointer(const struct nl_raw_data * const value, const struct nl_raw_data * const expected)
{
	if(expected == value) {
		if(value == NULL) {
			return 0;
		} else {
			ERROR_OUT("Expected and returned pointers are identical; they should differ\n");
			return -1;
		}
	}

	if(expected == NULL && value != NULL) {
		ERROR_OUT("Expected NULL pointer, got non-NULL pointer\n");
		return -1;
	}

	if(expected != NULL && value == NULL) {
		ERROR_OUT("Expected non-NULL pointer, got NULL pointer\n");
		return -1;
	}

	return check_struct(*value, *expected);
}

int main(void)
{
	const struct nl_raw_data hello = { .size = 6, .data = "Hello" };
	const struct nl_raw_data zero_not_null = { .size = 0, .data = "" };
	const struct nl_raw_data empty = { .size = 0, .data = NULL };
	const struct nl_raw_data invalid = { .size = 1, .data = NULL };
	const struct nl_raw_data error = { .size = SIZE_MAX, .data = NULL };

	struct nl_raw_data stack;
	struct nl_raw_data *heap;

	int ret = 0;

	// Test testing functions
	nl_ptmf("Testing test functions.  Expect some errors to be displayed.  %d error(s) so far.\n", ret);
	ret += !check_pointer(NULL, &empty);
	ret += !check_pointer(&(struct nl_raw_data){.size = 4, .data = "Hi!"}, &hello);

	// Test valid data copy
	nl_ptmf("Testing copy of good data.  %d error(s) so far.\n", ret);
	stack = nl_copy_data(hello);
	ret += !!check_struct(stack, hello);
	free(stack.data);

	// Test zero-sized data copy
	nl_ptmf("Testing copy of zero-sized data.  %d error(s) so far.\n", ret);
	stack = nl_copy_data(zero_not_null);
	ret += !!check_struct(stack, empty);

	// Test invalid data copy
	nl_ptmf("Testing copy of invalid data (expecting error).  %d error(s) so far.\n", ret);
	stack = nl_copy_data(invalid);
	ret += !!check_struct(stack, error);


	// Test valid data duplication
	nl_ptmf("Testing duplication of good data.  %d error(s) so far.\n", ret);
	heap = nl_duplicate_data(&hello);
	ret += !!check_pointer(heap, &hello);
	nl_destroy_data(heap);

	// Test zero-sized data duplication
	nl_ptmf("Testing duplication of zero-sized data.  %d error(s) so far.\n", ret);
	heap = nl_duplicate_data(&zero_not_null);
	ret += !!check_pointer(heap, &empty);
	nl_destroy_data(heap);

	// Test invalid data duplication
	nl_ptmf("Testing duplication of invalid data (expecting error).  %d error(s) so far.\n", ret);
	heap = nl_duplicate_data(&invalid);
	ret += !!check_pointer(heap, NULL);

	// Test NULL data duplication
	nl_ptmf("Testing duplication of NULL data (expecting error).  %d error(s) so far.\n", ret);
	heap = nl_duplicate_data(NULL);
	ret += !!check_pointer(heap, NULL);


	if(ret) {
		ERROR_OUT("\e[31m%d data tests failed.\e[0m\n", ret);
	} else {
		nl_ptmf("\e[32mAll data tests passed.\e[0m\n");
	}

	return ret;
}
