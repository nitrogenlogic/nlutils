/*
 * Tests struct nl_fifo.
 * Copyright (C)2009, 2014 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include "nlutils.h"
#include "fifo.h"


#define ERR(fifo, ...) {\
	ERROR_OUT(__VA_ARGS__);\
	ERROR_OUT("FIFO count: %d\n", fifo->count);\
}

int test_put(struct nl_fifo* fifo, void *put, unsigned int count)
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

int test_get(struct nl_fifo *fifo, char *expected, unsigned int count)
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

int test_peek(struct nl_fifo *fifo, char *expected, unsigned int count)
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

int test_remove(struct nl_fifo *fifo, void *remove, unsigned int count)
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

int test_next(struct nl_fifo *fifo, const struct nl_fifo_element **iter, char *expected)
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

void test_clear_cb(__attribute__((unused)) void *element, void *user_data)
{
	int *counter = user_data;
	printf("In the clear callback with %d\n", *counter);
	(*counter)++;
}

int main(void)
{
	char *str1 = "Test 1";
	char *str2 = "Test 2";
	char *str3 = "Test 3";

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

	// Test iteration
	INFO_OUT("Testing iteration on many-element FIFO.\n");
	iter = NULL;
	for(i = 0; i < 100; i++) {
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


	INFO_OUT("FIFO tests completed successfully.\n");

	return 0;
}

