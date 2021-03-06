/*
 * fifo.c - A generic FIFO implementation using a linked list.
 * Copyright (C)2009, 2014-2019 Mike Bourgeous.  Released under AGPLv3 in 2018.
 *
 * Copied from the logic system project.
 *
 * I implemented this because putting *next into every structure that will ever
 * get pushed onto a linked list can get annoying and wasteful, because the
 * logic system is to be implemented in straight C (no C++ STL), and because I
 * don't want to depend on a large library like libglib (in other words, this
 * has to be embedded friendly).
 *
 * If constant memory allocation becomes a problem, a pool of fifo entries
 * could be created (either globally or per fifo) with the pool growing as
 * needed, and perhaps shrinking when a fifo is destroyed.  It may also be more
 * efficient to use a reallocated array instead of a linked list, to improve
 * locality of access.
 *
 * Created Jul. 16, 2009, moved and updated Sept. 21, 2014 et seq (see Git).
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "nlutils.h"
#include "fifo.h"


/*
 * Single fifo element.
 */
struct nl_fifo_element {
	struct nl_fifo_element *next;
	void *data;
	struct nl_fifo *l;
};

/*
 * Error indicator used by nl_fifo_next
 */
static const struct nl_fifo_element fe;
const struct nl_fifo_element * const nl_fifo_error = &fe;

/*
 * Creates a new empty fifo.
 */
struct nl_fifo *nl_fifo_create()
{
	struct nl_fifo *l;

	l = calloc(1, sizeof(struct nl_fifo));
	if(l == NULL) {
		ERRNO_OUT("Error allocating new fifo");
		return NULL;
	}

	return l;
}

/*
 * Removes all elements from and destroys an existing fifo.  A NULL fifo is
 * ignored.
 */
void nl_fifo_destroy(struct nl_fifo *l)
{
	struct nl_fifo_element *cur, *next;

	if(l != NULL) {
		// Call nl_fifo_clear() here?  It would be slightly slower by a
		// couple of instructions, but would reduce code size
		cur = l->first;
		while(cur != NULL) {
			next = cur->next;
			free(cur);
			cur = next;
		}
		free(l);
	}
}

/*
 * Adds a new element to the fifo.  The return value is the number of elements
 * in the fifo after the new element is added, or negative on error.  NULL data
 * is considered to be an error.
 */
int nl_fifo_put(struct nl_fifo *l, void *data)
{
	struct nl_fifo_element *e;

	if(CHECK_NULL(l) || CHECK_NULL(data)) {
		return -1;
	}

	e = malloc(sizeof(struct nl_fifo_element));
	if(e == NULL) {
		ERRNO_OUT("Error allocating new fifo element");
		return -1;
	}
	e->data = data;
	e->next = NULL;
	e->l = l;

	// If it ever comes down to it, there's probably a slightly faster way
	// to organize the list, to avoid these conditionals.
	if(l->count == 0) {
		l->first = e;
		l->last = e;
	} else {
		l->last->next = e;
		l->last = e;
	}

	l->count++;

	return l->count;
}

/*
 * Removes the least-recently-added element from the fifo.  Returns NULL if the
 * list is empty or an error occurs.
 */
void *nl_fifo_get(struct nl_fifo *l)
{
	struct nl_fifo_element *e;
	void *data;

	if(CHECK_NULL(l)) {
		return NULL;
	}
	if(l->count == 0) {
		return NULL;
	}

	e = l->first;
	data = e->data;
	l->first = e->next;
	free(e);
	
	l->count--;

	return data;
}

/*
 * Retrieves, but does not remove the least-recently-added element from the
 * fifo.  Returns NULL if the list is empty or an error occurs.
 */
void *nl_fifo_peek(struct nl_fifo *l)
{
	if(CHECK_NULL(l)) {
		return NULL;
	}
	if(l->count == 0) {
		return NULL;
	}

	return l->first->data;
}

/*
 * Removes the least-recently-added instance of the given element from the
 * fifo.  Returns 0 if the element existed and was deleted, -1 if the element
 * did not exist or an error occurred.
 */
int nl_fifo_remove(struct nl_fifo *l, void *data)
{
	struct nl_fifo_element *cur, *prev;

	if(CHECK_NULL(l) || CHECK_NULL(data)) {
		return -1;
	}

	if(l->count == 0) {
		ERROR_OUT("Cannot remove an element from an empty FIFO; this is probably a bug.\n");
		return -1;
	}

	prev = NULL;
	cur = l->first;
	while(cur != NULL) {
		if(cur->data == data) {
			if(prev != NULL) {
				prev->next = cur->next;
			} else {
				l->first = cur->next;
			}
			if(l->last == cur) {
				l->last = prev;
			}
			l->count--;
			free(cur);

			return 0;
		}
		prev = cur;
		cur = cur->next;
	}

	return -1;
}

/*
 * Iterates through FIFO elements without altering the list.  For the first
 * call, *iter should be NULL.  For subsequent calls, *iter should be
 * unmodified.  The FIFO should be modifiable in the following ways while
 * iterating:
 * 
 *  - Removing any element before the current element
 *  - Removing any element after the current element
 *  - Adding an element to the end of the list (the new element will not be
 *  iterated if the last-returned element was the last element in the list)
 *
 * To avoid use-after-free bugs, remove the current element by storing it in a
 * *prev pointer while iterating, then removing it on the next iteration.  In
 * general, list modification during iteration should be avoided as it is
 * poorly tested.
 *
 * The behavior is undefined if nl_fifo_next() is called again with the same
 * parameters after the list end is reached.
 *
 * Returns NULL at list end.  Returns NULL and stores nl_fifo_error in *iter on
 * error (unless iter is NULL).
 */
void *nl_fifo_next(struct nl_fifo *l, const struct nl_fifo_element **iter)
{
	if(CHECK_NULL(l) || CHECK_NULL(iter)) {
		if(iter != NULL) {
			*iter = nl_fifo_error;
		}
		return NULL;
	}

	if(*iter == nl_fifo_error) {
		ERROR_OUT("Iterator is the error indicator, nl_fifo_error!\n");
		ERROR_OUT("nl_fifo_next() was called again after an error occurred.\n");
		*iter = nl_fifo_error;
		return NULL;
	}

	if(*iter != NULL && (*iter)->l != l) {
		ERROR_OUT("Iterator is from a different list!\n");
		*iter = nl_fifo_error;
		return NULL;
	}

	if(l->count == 0) {
		return NULL;
	}

	if(*iter == NULL) {
		*iter = l->first;
	} else {
		*iter = (*iter)->next;
	}

	return *iter == NULL ? NULL : (*iter)->data;
}

/*
 * Removes all elements from the FIFO.  Note that this does not free the
 * elements stored in the FIFO.
 *
 * This is not a thread-safe operation.
 */
void nl_fifo_clear(struct nl_fifo *l)
{
	nl_fifo_clear_cb(l, NULL, NULL);
}

/*
 * Removes all elements from the FIFO, calling the given callback (if not NULL)
 * for each element first.  This may be used e.g. to free memory.
 *
 * This is not a thread-safe operation.
 */
void nl_fifo_clear_cb(struct nl_fifo *l, void (*cb)(void *el, void *user_data), void *user_data)
{
	if(l != NULL) {
		struct nl_fifo_element *cur = l->first;
		struct nl_fifo_element *next;

		while(cur != NULL) {
			next = cur->next;
			if(cb != NULL) {
				cb(cur->data, user_data);
			}
			free(cur);
			cur = next;
		}

		l->first = NULL;
		l->last = NULL;
		l->count = 0;
	}
}
