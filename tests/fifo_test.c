/*
 * Tests struct nl_fifo.
 * Copyright (C)2009, 2014, 2022 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>

#include "nlutils.h"
#include "fifo.h"


#define ERR(fifo, ...) {\
	ERROR_OUT(__VA_ARGS__);\
	ERROR_OUT("FIFO count: %d\n", fifo->count);\
}

#define ASSERT_COUNT(fifo, expected_count, ...) do { \
	unsigned int fifo_count##__LINE__ = fifo ? fifo->count : 0; \
	if (fifo_count##__LINE__ != (expected_count)) { \
		ERROR_OUT(__VA_ARGS__); \
		ERROR_OUT_EX(": Expected fifo %p (%s) to have count %u (%s) but got count %u\n", \
				fifo, #fifo, (expected_count), #expected_count, fifo_count##__LINE__); \
		abort(); \
	} \
} while(0);

static int test_put(struct nl_fifo *fifo, void *put, unsigned int count)
{
	unsigned int oldcount = fifo->count;
	int error = 0;

	if(nl_fifo_put(fifo, put) != (int)oldcount + 1) {
		ERROR_OUT("nl_fifo_put() of %p failed.\n", put);
		error = -1;
	}

	if(fifo->count != oldcount + 1) {
		ERROR_OUT("Count unchanged or changed incorrectly from %d to %d after nl_fifo_put().\n",
				oldcount, fifo->count);
		error = -1;
	}

	if(fifo->count != count) {
		ERROR_OUT("Expected count of %d after nl_fifo_put(), actually %d.\n", count, fifo->count);
		error = -1;
	}

	return error;
}

static int test_prepend(struct nl_fifo *fifo, void *prepend, unsigned int count)
{
	unsigned int oldcount = fifo->count;
	int error = 0;

	if (nl_fifo_prepend(fifo, prepend) != (int)oldcount + 1) {
		ERROR_OUT("nl_fifo_prepend() of %p failed.\n", prepend);
		error = -1;
	}

	if (fifo->count != oldcount + 1) {
		ERROR_OUT("Count unchanged or changed incorrectly from %d to %d after nl_fifo_prepend().\n",
				oldcount, fifo->count);
		error = -1;
	}

	if (fifo->count != count) {
		ERROR_OUT("Expected count of %d after nl_fifo_prepend(), actually %d.\n", count, fifo->count);
		error = -1;
	}

	void *peek = nl_fifo_peek(fifo);
	if (peek != prepend) {
		ERROR_OUT("Expected fifo to start with newly prepended value %p, but got %p.\n", prepend, peek);
		error = -1;
	}

	return error;
}

static int test_get(struct nl_fifo *fifo, const char *expected, unsigned int count)
{
	char *result;
	unsigned int oldcount = fifo->count;
	int error = 0;

	result = nl_fifo_get(fifo);
	if(result != expected) {
		ERROR_OUT("nl_fifo_get() returned %s, expected %s.\n", result, expected);
		error = -1;
	}

	if(oldcount != 0 && fifo->count != oldcount - 1) {
		ERROR_OUT("Count unchanged or changed incorrectly from %d to %d after nl_fifo_get().\n",
				oldcount, fifo->count);
		error = -1;
	}

	if(fifo->count != count) {
		ERROR_OUT("Expected count of %d after nl_fifo_get(), actually %d.\n", count, fifo->count);
		error = -1;
	}

	return error;
}

static int test_peek(struct nl_fifo *fifo, char *expected, unsigned int count)
{
	char *result;
	unsigned int oldcount = fifo->count;
	int error = 0;

	result = nl_fifo_peek(fifo);
	if(result != expected) {
		ERROR_OUT("nl_fifo_peek() returned %s, expected %s.\n", result, expected);
		error = -1;
	}

	if(oldcount != fifo->count) {
		ERROR_OUT("Count changed from %d to %d after nl_fifo_peek().\n", oldcount, fifo->count);
		error = -1;
	}

	if(fifo->count != count) {
		ERROR_OUT("Expected count of %d after nl_fifo_peek(), actually %d.\n", count, fifo->count);
		error = -1;
	}

	return error;
}

static int test_remove(struct nl_fifo *fifo, void *remove, unsigned int count)
{
	unsigned int oldcount = fifo->count;
	int error = 0;

	if(nl_fifo_remove(fifo, remove)) {
		ERROR_OUT("nl_fifo_remove() of %p failed.\n", remove);
		error = -1;
	}

	if(oldcount != 0 && fifo->count != oldcount - 1) {
		ERROR_OUT("Count unchanged or changed incorrectly from %d to %d after nl_fifo_remove().\n",
				oldcount, fifo->count);
		error = -1;
	}

	if(fifo->count != count) {
		ERROR_OUT("Expected count of %d after nl_fifo_remove(), actually %d.\n", count, fifo->count);
		error = -1;
	}

	return error;
}

static int test_next(struct nl_fifo *fifo, const struct nl_fifo_element **iter, char *expected)
{
	int error = 0;
	char *result;

	result = nl_fifo_next(fifo, iter);
	if(*iter == nl_fifo_error) {
		ERROR_OUT("nl_fifo_next() indicated an error condition.\n");
		error = -1;
	}
	if(result != expected) {
		ERROR_OUT("nl_fifo_next() returned %s, expected %s.\n", result, expected);
		error = -1;
	}

	return error;
}

// Adds count elements containing data to the end of the fifo.
static void add_fifo_elements(struct nl_fifo *fifo, void *data, unsigned int count)
{
	unsigned int start = fifo->count;

	for (unsigned int i = 0; i < count; i++) {
		nl_fifo_put(fifo, data);
	}

	if (start + count != fifo->count) {
		ERROR_OUT("Expected %u elements in the fifo after adding %u to a start of %u, but ended with %u.\n",
				start + count, count, start, fifo->count);
		abort();
	}
}

struct removal_test {
	char *msg;
	unsigned int count;
	unsigned int error;
};

static void test_remove_cb(void *element, void *user_data)
{
	char *msg = element;
	struct removal_test *info = user_data;

	DEBUG_OUT("Entering the removal callback for %s with count %u\n", msg, info->count);

	if (msg != info->msg) {
		ERROR_OUT("Wrong message received: expected %s, got %s\n", info->msg, msg);
		info->error++;
	}

	info->count++;
}

static void test_remove_first(struct nl_fifo *fifo, unsigned int count, void (*cb)(void *el, void *user_data), void *user_data, unsigned int expected_count)
{
	unsigned int start = fifo ? fifo->count : 0;
	unsigned int expected_delta;

	if (count > start) {
		expected_delta = 0;
	} else {
		expected_delta = start - count;
	}

	unsigned int result = nl_fifo_remove_first(fifo, count, cb, user_data);

	ASSERT_COUNT(fifo, expected_delta, "delta after remove_first from %u with count %u", start, count);
	ASSERT_COUNT(fifo, expected_count, "final count after remove_first");
	ASSERT_COUNT(fifo, result, "fifo count matches return value from remove_first");
}

static void test_remove_last(struct nl_fifo *fifo, unsigned int count, void (*cb)(void *el, void *user_data), void *user_data, unsigned int expected_count)
{
	unsigned int start = fifo ? fifo->count : 0;
	unsigned int expected_delta;

	if (count > start) {
		expected_delta = 0;
	} else {
		expected_delta = start - count;
	}

	unsigned int result = nl_fifo_remove_last(fifo, count, cb, user_data);

	ASSERT_COUNT(fifo, expected_delta, "delta after remove_last from %u with count %u", start, count);
	ASSERT_COUNT(fifo, expected_count, "final count after remove_last");
	ASSERT_COUNT(fifo, result, "fifo count matches return value from remove_last");
}

static int test_remove_first_and_last(void)
{
	struct removal_test first_data = {
		.msg = "remove_first",
	};
	struct removal_test last_data = {
		.msg = "remove_last",
	};

	INFO_OUT("Testing nl_fifo_remove_first and nl_fifo_remove_last\n");

	struct nl_fifo *fifo = nl_fifo_create();
	if (CHECK_NULL(fifo)) {
		return -1;
	}

	INFO_OUT("\tTesting with NULL and empty lists\n");

	// Test remove_first on a NULL list (expect no errors or callback calls)
	// Test remove_first with a count of 0 on an empty list
	// Test remove_first with a nonzero count on an empty list
	test_remove_first(NULL, 50, test_remove_cb, &first_data, 0);
	test_remove_first(fifo, 0, test_remove_cb, &first_data, 0);
	test_remove_first(fifo, 50, test_remove_cb, &first_data, 0);

	// Test remove_last on a NULL list (expect no errors or callback calls)
	// Test remove_last with a count of 0 on an empty list
	// Test remove_last with a nonzero count on an empty list
	test_remove_last(NULL, 50, test_remove_cb, &last_data, 0);
	test_remove_last(fifo, 0, test_remove_cb, &last_data, 0);
	test_remove_last(fifo, 50, test_remove_cb, &last_data, 0);


	INFO_OUT("\tTesting remove_first\n");
	add_fifo_elements(fifo, first_data.msg, 50);
	ASSERT_COUNT(fifo, 50, "add for remove_first");

	// Test remove_first without a callback
	// Test remove_first with a callback
	// Test remove_first with a count of 0
	// Test remove_first with a count equal to the list size
	// Test remove_first with a count greater than the list size
	test_remove_first(fifo, 2, NULL, NULL, 48);
	test_remove_first(fifo, 1, test_remove_cb, &first_data, 47);
	test_remove_first(fifo, 5, test_remove_cb, &first_data, 42);
	test_remove_first(fifo, 0, test_remove_cb, &first_data, 42);
	test_remove_first(fifo, 42, test_remove_cb, &first_data, 0);
	add_fifo_elements(fifo, first_data.msg, 50);
	test_remove_first(fifo, 60, test_remove_cb, &first_data, 0);

	const unsigned int expected_first = 48 + 50;
	if (first_data.count != expected_first) {
		ERROR_OUT("Expected remove_first callback to be called %u times, but got %u\n", expected_first, first_data.count);
		return -1;
	}
	if (first_data.error != 0) {
		ERROR_OUT("Callback for remove_first set error to %u (expected 0)\n", first_data.error);
		return -1;
	}


	INFO_OUT("\tTesting remove_last\n");
	add_fifo_elements(fifo, last_data.msg, 50);
	ASSERT_COUNT(fifo, 50, "add for remove_last");

	// Test remove_last without a callback
	// Test remove_last with a callback
	// Test remove_last with a count of 0
	// Test remove_last with a count equal to the list size
	// Test remove_last with a count greater than the list size
	test_remove_last(fifo, 2, NULL, NULL, 48);
	test_remove_last(fifo, 1, test_remove_cb, &last_data, 47);
	test_remove_last(fifo, 5, test_remove_cb, &last_data, 42);
	test_remove_last(fifo, 0, test_remove_cb, &last_data, 42);
	test_remove_last(fifo, 42, test_remove_cb, &last_data, 0);
	add_fifo_elements(fifo, last_data.msg, 50);
	test_remove_last(fifo, 60, test_remove_cb, &last_data, 0);

	const unsigned int expected_last = 48 + 50;
	if (last_data.count != expected_last) {
		ERROR_OUT("Expected remove_last callback to be called %u times, but got %u\n", expected_last, last_data.count);
		return -1;
	}
	if (last_data.error != 0) {
		ERROR_OUT("Callback for remove_last set error to %u (expected 0)\n", last_data.error);
		return -1;
	}

	INFO_OUT("Testing a mix of remove_first, remove_last, and addition (to check internal pointers)\n");
	add_fifo_elements(fifo, first_data.msg, 5);
	add_fifo_elements(fifo, last_data.msg, 5);
	test_remove_first(fifo, 2, test_remove_cb, &first_data, 8);
	test_remove_last(fifo, 2, test_remove_cb, &last_data, 6);

	test_put(fifo, last_data.msg, 7);
	test_prepend(fifo, first_data.msg, 8);
	test_remove_first(fifo, 2, test_remove_cb, &first_data, 6);
	test_remove_last(fifo, 2, test_remove_cb, &last_data, 4);
	test_remove_last(fifo, 2, test_remove_cb, &last_data, 2);
	test_remove_last(fifo, 1, test_remove_cb, &first_data, 1); // At this point last_data should all be gone

	test_prepend(fifo, first_data.msg, 2);
	test_put(fifo, last_data.msg, 3);
	test_remove_first(fifo, 2, test_remove_cb, &first_data, 1);
	test_remove_first(fifo, 1, test_remove_cb, &last_data, 0);

	if (first_data.error != 0 || last_data.error != 0) {
		ERROR_OUT("Callback for remove_first or remove_last set error: first=%u last=%u (expected 0)\n",
				first_data.error, last_data.error);
		return -1;
	}
	nl_fifo_destroy(fifo);

	return 0;
}

static void test_concat_start(struct nl_fifo *src, struct nl_fifo *dest, unsigned int expected)
{
	unsigned int src_start = src ? src->count : 0;
	unsigned int dest_start = dest ? dest->count : 0;
	unsigned int dest_expect = dest ? src_start + dest_start : 0;

	unsigned int result = nl_fifo_concat_start(src, dest);

	ASSERT_COUNT(dest, result, "dest count should match return value");
	ASSERT_COUNT(src, dest ? 0 : src_start, "source should be empty unless dest was null");
	ASSERT_COUNT(dest, expected, "expecting a count of %u", expected);
	ASSERT_COUNT(dest, dest_expect,
			"expecting dest count to be sum of old count %u and src count %u",
			dest_start, src_start);
}

static void test_concat_end(struct nl_fifo *src, struct nl_fifo *dest, unsigned int expected)
{
	unsigned int src_start = src ? src->count : 0;
	unsigned int dest_start = dest ? dest->count : 0;
	unsigned int dest_expect = dest ? src_start + dest_start : 0;

	unsigned int result = nl_fifo_concat_end(src, dest);

	ASSERT_COUNT(dest, result, "dest count should match return value");
	ASSERT_COUNT(src, dest ? 0 : src_start, "source should be empty unless dest was null");
	ASSERT_COUNT(dest, expected, "expecting a count of %u", expected);
	ASSERT_COUNT(dest, dest_expect,
			"expecting dest count to be sum of old count %u and src count %u",
			dest_start, src_start);
}

static int test_concat(void)
{
	char *str1 = "first";
	char *str2 = "second";
	char *str3 = "third";

	INFO_OUT("Testing nl_fifo_concat_start and nl_fifo_concat_end\n");

	struct nl_fifo *f1 = nl_fifo_create();
	struct nl_fifo *f2 = nl_fifo_create();
	struct nl_fifo *f3 = nl_fifo_create();
	if (CHECK_NULL(f1) || CHECK_NULL(f2) || CHECK_NULL(f3)) {
		ERRNO_OUT("Error allocating fifos");
		return -1;
	}


	INFO_OUT("\tTesting with NULL and empty lists\n");
	test_concat_start(NULL, NULL, 0);
	test_concat_start(NULL, f2, 0);
	test_concat_start(f1, NULL, 0);
	test_concat_start(f1, f2, 0);
	test_concat_end(NULL, NULL, 0);
	test_concat_end(NULL, f2, 0);
	test_concat_end(f1, NULL, 0);
	test_concat_end(f1, f2, 0);


	INFO_OUT("\tTesting nl_fifo_concat_start\n");
	add_fifo_elements(f1, str1, 5);
	add_fifo_elements(f2, str2, 5);
	test_concat_start(f1, f2, 10);
	test_get(f1, NULL, 0);
	test_get(f2, str1, 9);
	test_concat_start(f2, f1, 9);
	test_get(f1, str1, 8);
	test_get(f2, NULL, 0);

	// null and empty
	test_concat_start(f3, f1, 8);
	test_concat_start(NULL, f1, 8);
	test_concat_start(f1, NULL, 0);

	const struct nl_fifo_element *iter = NULL;
	void *data;
	unsigned int iter_count = 0;
	while ((data = nl_fifo_next(f1, &iter))) {
		// This loop just makes sure iteration still works across both lists
		DEBUG_OUT("\t\tGot %s\n", (char *)data);
		iter_count += 1;
	}
	ASSERT_COUNT(f1, iter_count, "Should iterate over all elements after concat_start");

	for (int i = 0; i < 3; i++) {
		test_get(f1, str1, 7 - i);
	}
	for (int i = 0; i < 5; i++) {
		test_get(f1, str2, 4 - i);
	}


	INFO_OUT("\tTesting nl_fifo_concat_end\n");
	add_fifo_elements(f1, str1, 5);
	add_fifo_elements(f2, str2, 5);
	test_concat_end(f1, f2, 10);
	test_get(f1, NULL, 0);
	test_get(f2, str2, 9);
	test_concat_end(f2, f1, 9);
	test_get(f1, str2, 8);
	test_get(f2, NULL, 0);

	// null and empty
	test_concat_end(f3, f1, 8);
	test_concat_end(NULL, f1, 8);
	test_concat_end(f1, NULL, 0);

	iter = NULL;
	iter_count = 0;
	while ((data = nl_fifo_next(f1, &iter))) {
		// This loop just makes sure iteration still works across both lists
		DEBUG_OUT("\t\tGot %s\n", (char *)data);
		iter_count += 1;
	}
	ASSERT_COUNT(f1, iter_count, "Should iterate over all elements after concat_end");

	for (int i = 0; i < 3; i++) {
		test_get(f1, str2, 7 - i);
	}
	for (int i = 0; i < 5; i++) {
		test_get(f1, str1, 4 - i);
	}


	INFO_OUT("\tTesting a mix of concat_start and concat_end\n");
	add_fifo_elements(f1, str1, 2);
	add_fifo_elements(f2, str2, 1);
	add_fifo_elements(f3, str3, 3);

	test_concat_start(f2, f1, 3);
	test_concat_end(f3, f1, 6);
	test_concat_start(f2, f1, 6);
	test_concat_end(f3, f1, 6);
	test_concat_start(f1, f2, 6);
	test_concat_end(f2, f1, 6);

	test_get(f1, str1, 5);
	test_get(f1, str1, 4);
	test_get(f1, str3, 3);
	test_get(f1, str3, 2);
	test_get(f1, str3, 1);
	test_get(f1, str1, 0);

	nl_fifo_destroy(f3);
	nl_fifo_destroy(f2);
	nl_fifo_destroy(f1);

	return 0;
}

static void test_clear_cb(__attribute__((unused)) void *element, void *user_data)
{
	int *counter = user_data;
	DEBUG_OUT("In the clear callback with %d\n", *counter);
	(*counter)++;
}

int main(void)
{
	char *str1 = "Test 1";
	char *str2 = "Test 2";
	char *str3 = "Test 3";
	char *str4 = "Test 4";

	const struct nl_fifo_element *iter;
	struct nl_fifo *f1, *f2;
	int i;

	int clear_counter;

	// Test destruction immediately after creation with no manipulation
	INFO_OUT("Testing destroying unmodified FIFO.\n");
	f1 = nl_fifo_create();
	if(CHECK_NULL(f1)) {
		return -1;
	}
	nl_fifo_destroy(f1);

	// Test basic operations/errors
	INFO_OUT("Testing basic FIFO operations and errors.\n");
	f1 = nl_fifo_create();
	if(CHECK_NULL(f1)) {
		return -1;
	}

	if(nl_fifo_peek(f1) != NULL) {
		ERR(f1, "Non-null result for peek on an empty FIFO.\n");
		return -1;
	}
	if(nl_fifo_peek(NULL) != NULL) {
		ERR(f1, "Non-null result for peek on a NULL FIFO.\n");
		return -1;
	}

	if(!nl_fifo_put(f1, NULL)) {
		ERR(f1, "No error adding NULL to FIFO.\n");
		return -1;
	}

	if(nl_fifo_get(NULL)) {
		ERR(f1, "No error getting from a NULL FIFO.\n");
		return -1;
	}

	if(!nl_fifo_put(NULL, str1) || !nl_fifo_put(NULL, NULL)) {
		ERR(f1, "No error adding to a NULL FIFO.\n");
		return -1;
	}

	if(test_get(f1, NULL, 0)) {
		ERR(f1, "Empty FIFO returned non-NULL for nl_fifo_get().\n");
		return -1;
	}

	if(!nl_fifo_remove(NULL, str1)) {
		ERR(f1, "No error removing from a NULL FIFO.\n");
		return -1;
	}
	if(!nl_fifo_remove(NULL, NULL)) {
		ERR(f1, "No error removing NULL from a NULL FIFO.\n");
		return -1;
	}

	if(!nl_fifo_remove(f1, str1)) {
		ERR(f1, "No error removing from an empty FIFO.\n");
		return -1;
	}

	if(test_put(f1, str1, 1)) {
		ERR(f1, "Error adding %s to FIFO.\n", str1);
		return -1;
	}

	if(test_peek(f1, str1, 1)) {
		ERR(f1, "Error peeking single-element FIFO.\n");
		return -1;
	}

	if(test_put(f1, str2, 2)) {
		ERR(f1, "Error adding %s to FIFO.\n", str2);
		return -1;
	}

	if(test_peek(f1, str1, 2)) {
		ERR(f1, "Error peeking two-element FIFO.\n");
		return -1;
	}

	if(!nl_fifo_remove(f1, NULL)) {
		ERR(f1, "No error removing NULL from a FIFO.\n");
		return -1;
	}

	if(test_get(f1, str1, 1)) {
		ERR(f1, "Error pulling first element from two-element FIFO.\n");
		return -1;
	}
	if(!nl_fifo_remove(f1, str1) || f1->count != 1) {
		ERR(f1, "No error removing nonexistent element from FIFO.\n");
		return -1;
	}
	iter = NULL;
	if(test_next(f1, &iter, str2) || test_next(f1, &iter, NULL)) {
		ERR(f1, "Error during iteration of single-element list.\n");
		return -1;
	}
	if(test_get(f1, str2, 0)) {
		ERR(f1, "Error pulling first element from now-single-element FIFO.\n");
		return -1;
	}

	if(!nl_fifo_remove(f1, str1)) {
		ERR(f1, "No error removing from an empty FIFO.\n");
		return -1;
	}

	// Test removal of the sole element
	if(test_put(f1, str1, 1)) {
		ERR(f1, "Error putting %s.\n", str1);
		return -1;
	}
	if(test_remove(f1, str1, 0)) {
		ERR(f1, "Error removing %s from single-element FIFO.\n", str1);
		return -1;
	}

	iter = NULL;
	if(test_next(f1, &iter, NULL)) {
		ERR(f1, "Iteration result not NULL on empty FIFO.\n");
		return -1;
	}

	// Ensure no crash occurs when clearing an empty FIFO with or without callback
	clear_counter = 0;
	nl_fifo_clear(f1);
	nl_fifo_clear_cb(f1, test_clear_cb, &clear_counter);
	if(clear_counter != 0) {
		ERR(f1, "Clearing 0 elements resulted in change to clear counter.\n");
		return -1;
	}

	// Add/remove many elements to/from the FIFO
	INFO_OUT("Testing many element sequencing.\n");
	if(test_put(f1, str2, 1)) {
		ERR(f1, "Error adding %s to FIFO.\n", str2);
		return -1;
	}
	for(i = 0; i < 100; i++) {
		if(test_put(f1, str1, i + 2)) {
			ERR(f1, "Error adding %s to FIFO.\n", str1);
			return -1;
		}
	}
	if(test_remove(f1, str2, 100)) {
		ERR(f1, "Error removing the first entry (%s) from FIFO.\n", str2);
		return -1;
	}
	if(test_put(f1, str2, 101)) {
		ERR(f1, "Error adding %s to FIFO.\n", str2);
		return -1;
	}
	for(i = 0; i < 100; i++) {
		if(test_put(f1, str3, i + 102)) {
			ERR(f1, "Error adding %s to FIFO.\n", str3);
			return -1;
		}
	}
	if(test_put(f1, str2, 202)) {
		ERR(f1, "Error adding %s to FIFO.\n", str2);
		return -1;
	}

	INFO_OUT("Testing removal on many-element FIFO.\n");

	for (i = 0; i < 5; i++) {
		if (test_prepend(f1, str4, i + 203)) {
			ERR(f1, "Error prepending %s to FIFO.\n", str4);
			return -1;
		}
	}
	if (test_remove(f1, str1, 206)) {
		ERR(f1, "Error removing the first %s from the FIFO.\n", str1);
		return -1;
	}
	if (test_remove(f1, str4, 205)) {
		ERR(f1, "Error removing the first element %s from the FIFO.\n", str4);
		return -1;
	}

	// Test iteration
	INFO_OUT("Testing iteration on many-element FIFO.\n");
	iter = NULL;
	for (i = 0; i < 4; i++) {
		if (test_next(f1, &iter, str4)) {
			ERR(f1, "Error iterating over copies of %s.\n", str4);
			return -1;
		}
	}
	for(i = 0; i < 99; i++) {
		if(test_next(f1, &iter, str1)) {
			ERR(f1, "Error iterating over copies of %s.\n", str1);
			return -1;
		}
	}
	if(test_next(f1, &iter, str2)) {
		ERR(f1, "Error iterating over copy of %s.\n", str2);
		return -1;
	}
	for(i = 0; i < 100; i++) {
		if(test_next(f1, &iter, str3)) {
			ERR(f1, "Error iterating over copies of %s.\n", str3);
			return -1;
		}
	}
	if(test_next(f1, &iter, str2)) {
		ERR(f1, "Error iterating over copy of %s.\n", str2);
		return -1;
	}
	if(test_next(f1, &iter, NULL)) {
		ERR(f1, "Result not NULL at end of list.\n");
		return -1;
	}

	INFO_OUT("Testing removal on many-element FIFO.\n");

	// Revert the changes made by prepend testing
	for (i = 0; i < 4; i++) {
		if (test_get(f1, str4, 204 - i)) {
			ERR(f1, "Error removing first entry from FIFO.\n");
			return -1;
		}
	}
	if (test_prepend(f1, str1, 202)) {
		ERR(f1, "Error prepending to FIFO.\n");
		return -1;
	}

	if(test_get(f1, str1, 201)) {
		ERR(f1, "Error removing first entry from FIFO.\n");
		return -1;
	}

	if(test_remove(f1, str3, 200)) {
		ERR(f1, "Error removing a middle entry (%s) from FIFO.\n", str3);
		return -1;
	}

	if(test_remove(f1, str2, 199)) {
		ERR(f1, "Error removing a middle entry (%s) from FIFO.\n", str2);
		return -1;
	}

	for(i = 0; i < 99; i++) {
		if(test_get(f1, str1, 198 - i)) {
			ERR(f1, "Error getting %s from FIFO.\n", str1);
			return -1;
		}
	}
	for(i = 0; i < 99; i++) {
		if(test_get(f1, str3, 99 - i)) {
			ERR(f1, "Error getting %s from FIFO.\n", str3);
			return -1;
		}
	}

	if(test_get(f1, str2, 0)) {
		ERR(f1, "Error getting %s from FIFO -- it should not have been removed.\n", str2);
		return -1;
	}

	nl_fifo_destroy(f1);

	// Test destruction of a filled FIFO after some removals
	INFO_OUT("Testing destroying a filled FIFO.\n");
	f1 = nl_fifo_create();
	if(CHECK_NULL(f1)) {
		return -1;
	}

	for(i = 0; i < 100; i++) {
		if(test_put(f1, str1, i * 3 + 1) ||
				test_put(f1, str2, i * 3 + 2) ||
				test_put(f1, str3, i * 3 + 3)) {
			ERR(f1, "Error adding %s, %s, or %s\n", str1, str2, str3);
			return -1;
		}

		if(test_peek(f1, str1, i * 3 + 3)) {
			ERR(f1, "Error peeking %s\n", str1);
			return -1;
		}
	}

	for(i = 0; i < 12; i++) {
		if(test_remove(f1, str2, 299 - i)) {
			ERR(f1, "Error removing %s\n", str2);
			return -1;
		}
	}

	nl_fifo_destroy(f1);

	// Make sure no crash occurs on destruction of NULL FIFO
	nl_fifo_destroy(NULL);


	// Test clearing a filled FIFO
	INFO_OUT("Testing clearing a filled FIFO.\n");
	f1 = nl_fifo_create();
	if(CHECK_NULL(f1)) {
		return -1;
	}

	for(i = 0; i < 100; i++) {
		if(test_put(f1, str1, i + 1)) {
			ERR(f1, "Error adding %s to FIFO.\n", str1);
			return -1;
		}
	}

	clear_counter = 0;
	nl_fifo_clear_cb(f1, test_clear_cb, &clear_counter);
	if(f1->count != 0) {
		ERR(f1, "Count is not zero after clearing.\n");
		return -1;
	}
	if(clear_counter != 100) {
		ERR(f1, "Element callback was called %d times instead of 100\n", clear_counter);
		return -1;
	}

	nl_fifo_destroy(f1);

	// Test modification during iteration
	INFO_OUT("Testing basic list modification during iteration (to be avoided!).\n");
	f1 = nl_fifo_create();
	if(CHECK_NULL(f1)) {
		return -1;
	}

	for(i = 0; i < 100; i++) {
		if(test_put(f1, str1, i + 1)) {
			ERR(f1, "Error adding %s to FIFO.\n", str1);
			return -1;
		}
	}

	for(i = 0, iter = NULL; i < 25; i++) {
		if(test_next(f1, &iter, str1)) {
			ERR(f1, "Error iterating over initial %s copies.\n", str1);
			return -1;
		}
	}
	if(test_put(f1, str2, 101)) {
		ERR(f1, "Error adding %s during iteration.\n", str2);
		return -1;
	}
	if(test_put(f1, str2, 102)) {
		ERR(f1, "Error adding %s during iteration.\n", str2);
		return -1;
	}
	for(i = 25; i < 50; i++) {
		if(test_next(f1, &iter, str1)) {
			ERR(f1, "Error iterating over %s copies.\n", str1);
			return -1;
		}
	}
	if(test_remove(f1, str1, 101)) {
		ERR(f1, "Error removing %s during iteration.\n", str1);
		return -1;
	}
	for(i = 50; i < 75; i++) {
		if(test_next(f1, &iter, str1)) {
			ERR(f1, "Error iterating over %s copies.\n", str1);
			return -1;
		}
	}
	if(test_remove(f1, str2, 100)) {
		ERR(f1, "Error removing post-iterator %s during iteration.\n", str2);
		return -1;
	}
	if(test_put(f1, str3, 101)) {
		ERR(f1, "Error adding %s during iteration.\n", str3);
		return -1;
	}
	if(test_put(f1, str3, 102)) {
		ERR(f1, "Error adding %s during iteration.\n", str3);
		return -1;
	}
	if(test_remove(f1, str1, 101)) {
		ERR(f1, "Error removing %s during iteration.\n", str1);
		return -1;
	}
	if(test_remove(f1, str3, 100)) {
		ERR(f1, "Error removing %s during iteration.\n", str1);
		return -1;
	}
	for(i = 75; i < 100; i++) {
		if(test_next(f1, &iter, str1)) {
			ERR(f1, "Error iterating over final %s copies.\n", str1);
			return -1;
		}
	}
	if(test_next(f1, &iter, str2)) {
		ERR(f1, "Error iterating over %s at list end.\n", str2);
		return -1;
	}
	if(test_next(f1, &iter, str3)) {
		ERR(f1, "Error iterating over %s at list end.\n", str3);
		return -1;
	}
	if(test_remove(f1, str2, 99)) {
		ERR(f1, "Error removing %s just after its iteration.\n", str2);
		return -1;
	}
	if(test_next(f1, &iter, NULL)) {
		ERR(f1, "Error getting NULL at end of iteration.\n");
		return -1;
	}

	// Test iteration errors
	INFO_OUT("Testing iteration errors.\n");
	iter = NULL;
	if(nl_fifo_next(NULL, &iter) != NULL || iter != nl_fifo_error) {
		ERR(f1, "No error iterating over a NULL FIFO.\n");
		return -1;
	}

	if(nl_fifo_next(f1, NULL) != NULL) {
		ERR(f1, "Non-null return when iter is NULL.\n");
		return -1;
	}

	iter = nl_fifo_error;
	if(nl_fifo_next(f1, &iter) != NULL || iter != nl_fifo_error) {
		ERR(f1, "No error iterating with a nl_fifo_error iterator.\n");
		return -1;
	}

	// Test using an iterator from the wrong list
	f2 = nl_fifo_create();
	if(CHECK_NULL(f2)) {
		return -1;
	}
	if(test_put(f2, str1, 1)) {
		ERR(f2, "Error adding %s to second FIFO.\n", str1);
		return -1;
	}

	iter = NULL;
	if(test_next(f1, &iter, str1)) {
		ERR(f1, "Error starting new iterator expecting %s.\n", str1);
		return -1;
	}

	if(nl_fifo_next(f2, &iter) != NULL || iter != nl_fifo_error) {
		ERR(f2, "No error using wrong list's iterator.\n");
		return -1;
	}

	nl_fifo_destroy(f1);
	nl_fifo_destroy(f2);


	// Test removing the previous element during iteration
	INFO_OUT("Testing repeated removal of previous element during iteration\n");
	f1 = nl_fifo_create();
	if(CHECK_NULL(f1)) {
		return -1;
	}

	for(i = 1; i <= 10000; i++) {
		if(test_put(f1, (void *)(uintptr_t)i, i)) {
			ERR(f1, "Error adding %p to FIFO.\n", (void *)(uintptr_t)i);
			return -1;
		}
	}

	void *prev = NULL, *cur = NULL;
	iter = NULL;
	i = 0;
	do {
		prev = cur;
		cur = nl_fifo_next(f1, &iter);

		if(prev != NULL && test_remove(f1, prev, 10000 - i)) {
			ERR(f1, "Error removing %p from FIFO.\n", prev);
			return -1;
		}

		i++;
	} while(cur);

	if(f1->count != 0) {
		ERR(f1, "Expected FIFO to be empty after removing all previous elements.\n");
		return -1;
	}

	nl_fifo_destroy(f1);


	// Test clearing a NULL fifo (double-check behavior with Valgrind to be sure)
	nl_fifo_clear(NULL);


	// Test remove_first and remove_last functions
	if (test_remove_first_and_last()) {
		return -1;
	}

	if (test_concat()) {
		return -1;
	}

	INFO_OUT("FIFO tests completed successfully.\n");

	return 0;
}

