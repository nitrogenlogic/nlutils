/*
 * Tests memory management functions.
 * Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "nlutils.h"

#define CREALLOC_FAIL(...) do { \
	ERROR_OUT(__VA_ARGS__); \
	ERROR_OUT("Params: ptr=%p, *ptr=%p, size=%zu, old=%zu, new=%zu\n", \
			ptr, orig, (size_t)size, (size_t)old, (size_t)new); \
	ERROR_OUT("New *ptr: %p, Result: %p\n", *ptr, result); \
	abort(); \
} while(0);

// Returns the number of bytes at *ptr that match value.
static size_t count_bytes(char *ptr, char value, size_t max)
{
	size_t s;

	for(s = 0; s < max && *ptr == value; s++) {
		ptr++;
	}

	return s;
}

// Stores test data, calls nl_crealloc(), then validates test data, cleared
// data, and result based on parameters given.
static void do_crealloc(void **ptr, uint64_t size, uint64_t old, uint64_t new)
{
	void *orig = *ptr;
	void *result = NULL;
	size_t scansize;

	DEBUG_OUT("nl_crealloc params: ptr=%p, *ptr=%p, size=%zu, old=%zu, new=%zu\n",
			ptr, orig, (size_t)size, (size_t)old, (size_t)new);

	if(size == 0) {
		CREALLOC_FAIL("Pass a nonzero size\n");
	}

	if(*ptr != NULL && size != INT_MAX) {
		memset(*ptr, 'Z', size * old);
	}

	result = nl_crealloc(ptr, size, old, new);

	if(size * new > 0) {
		if(size == INT_MAX) {
			// Special case to expect failure
			if(result != NULL) {
				CREALLOC_FAIL("nl_crealloc should have returned NULL for giant alloc\n");
			}
			if(*ptr != orig) {
				CREALLOC_FAIL("nl_crealloc should not have modified *ptr on failure\n");
			}

			return;
		} else {
			if(result == NULL) {
				CREALLOC_FAIL("nl_crealloc should have returned non-NULL for positive size\n");
			}
		}
	} else if(result != NULL) {
		CREALLOC_FAIL("nl_crealloc should have returned NULL for resize to 0\n");
	}

	if(result != *ptr) {
		CREALLOC_FAIL("nl_crealloc return value differs from stored *ptr\n");
	}

	if(old > 0 && new > 0) {
		scansize = count_bytes(*ptr, 'Z', size * new);
		if(scansize != size * MIN_NUM(new, old)) {
			CREALLOC_FAIL("nl_crealloc expected %zu bytes of 'Z', got %zu\n", (size_t)(size * old), (size_t)scansize);
		}
	}

	if(new > old) {
		scansize = count_bytes(*ptr + size * old, 0, size * (new - old));
		if(scansize != size * (new - old)) {
			CREALLOC_FAIL("nl_crealloc expected %zu bytes of 0, got %zu\n", (size_t)(size * (new - old)), (size_t)scansize);
		}
	}
}

static void test_nl_crealloc()
{
	void *ptr = NULL;

	INFO_OUT("Testing initial allocation\n");
	do_crealloc(&ptr, 32, 0, 1);

	if(ptr == NULL) {
		ERROR_OUT("Pointer should not be NULL after allocation\n");
		abort();
	}

	INFO_OUT("Testing growth\n");
	do_crealloc(&ptr, 32, 1, 10);

	INFO_OUT("Testing shrinking\n");
	do_crealloc(&ptr, 32, 10, 3);

	if(sizeof(size_t) > 4) {
		INFO_OUT("Testing failure (large size_t)\n");
		do_crealloc(&ptr, INT_MAX, 0x10000000, 0x10000000);
	} else {
		INFO_OUT("Testing failure (small size_t)\n");
		do_crealloc(&ptr, INT_MAX, 1, 2);
	}

	INFO_OUT("Testing no change\n");
	do_crealloc(&ptr, 32, 3, 3);

	INFO_OUT("Testing another shrink\n");
	do_crealloc(&ptr, 32, 3, 2);

	INFO_OUT("Testing yet another shrink\n");
	do_crealloc(&ptr, 32, 2, 1);

	INFO_OUT("Testing another grow\n");
	do_crealloc(&ptr, 32, 1, 4);

	INFO_OUT("Testing yet another grow\n");
	do_crealloc(&ptr, 32, 4, 5);

	INFO_OUT("Testing deallocation\n");
	nl_crealloc(&ptr, 32, 5, 0);

	if(ptr != NULL) {
		ERROR_OUT("Pointer should have been NULL after deallocation.\n");
		abort();
	}
}

int main(void)
{
	test_nl_crealloc();
	return 0;
}
