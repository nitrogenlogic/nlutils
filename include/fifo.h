/*
 * fifo.h - A generic FIFO implementation using a linked list.
 * Copyright (C)2009, 2014-2019 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_FIFO_H_
#define NLUTILS_FIFO_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 * Internal FIFO data structure.  Fields should not be modified by the user,
 * and only count should be read.
 */
struct nl_fifo_element;
struct nl_fifo {
	struct nl_fifo_element *first;	// First will always have a valid element, or NULL
	struct nl_fifo_element *last;	// Last may have invalid data if count==0
	unsigned int count; // TODO: make this size_t or ssize_t
};

/*
 * Constant used by nl_fifo_next() to indicate an error condition.
 */
extern const struct nl_fifo_element * const nl_fifo_error;


/*
 * Creates a new empty fifo.
 */
struct nl_fifo *nl_fifo_create();

/*
 * Removes all elements from and destroys an existing fifo.  A NULL fifo is
 * ignored.
 */
void nl_fifo_destroy(struct nl_fifo *l);

/*
 * Adds a new element to the end of the fifo.  The return value is the number
 * of elements in the fifo after the new element is added, or negative on
 * error.  NULL data is considered to be an error.
 */
int nl_fifo_put(struct nl_fifo *l, void *data);

/*
 * Prepends an element to the beginning of the fifo.  The return value is the
 * number of elements in the fifo after the new element is added, or negative
 * on error.  NULL data is considered to be an error.
 */
int nl_fifo_prepend(struct nl_fifo *l, void *data);

/*
 * Removes the least-recently-added element from the fifo.  Returns NULL if the
 * list is empty or an error occurs.
 */
void *nl_fifo_get(struct nl_fifo *l);

/*
 * Retrieves, but does not remove the least-recently-added element from the
 * fifo.  Returns NULL if the list is empty or an error occurs.
 */
void *nl_fifo_peek(struct nl_fifo *l);

/*
 * Removes the least-recently-added instance of the given element from the
 * fifo.  Returns 0 if the element existed and was deleted, -1 if the element
 * did not exist or an error occurred.  NULL data is an error.
 */
int nl_fifo_remove(struct nl_fifo *l, void *data);

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
void *nl_fifo_next(struct nl_fifo *l, const struct nl_fifo_element **iter);

/*
 * Removes all elements from the FIFO.  Note that this does not free the
 * elements stored in the FIFO.
 *
 * This is not a thread-safe operation.
 */
void nl_fifo_clear(struct nl_fifo *l);

/*
 * Removes all elements from the FIFO, calling the given callback (if not NULL)
 * for each element before its removal.  This may be used e.g. to free memory.
 *
 * This is not a thread-safe operation.
 */
void nl_fifo_clear_cb(struct nl_fifo *l, void (*cb)(void *el, void *user_data), void *user_data);

/*
 * Removes the first count elements from the FIFO, calling the given callback
 * (if not NULL) for each element before its removal.  Returns the number of
 * elements remaining after removal, or 0 if the fifo was NULL.
 *
 * This is not a thread-safe operation.
 */
unsigned int nl_fifo_remove_start(struct nl_fifo *l, unsigned int count, void (*cb)(void *el, void *user_data), void *user_data);

/*
 * Removes the last count elements from the FIFO, calling the given callback
 * (if not NULL) for each element before its removal.  Returns the number of
 * elements remaining after removal, or 0 if the fifo was NULL.
 *
 * This is not a thread-safe operation.
 */
unsigned int nl_fifo_remove_end(struct nl_fifo *l, unsigned int count, void (*cb)(void *el, void *user_data), void *user_data);

/*
 * Removes all elements from src and prepends them to the beginning of dest.
 * Afterward, nl_fifo_get(dest) would return all the elements from src first,
 * then all the elements from dest.  Returns the resulting number of elements
 * in dest.
 */
unsigned int nl_fifo_concat_start(struct nl_fifo *src, struct nl_fifo *dest);

/*
 * Removes all elements from src and concatenates them to the end of dest.
 * Afterward, nl_fifo_get(dest) would return all the elements from dest first,
 * then all the elements from src.  Returns the resulting number of elements in
 * dest.
 */
unsigned int nl_fifo_concat_end(struct nl_fifo *src, struct nl_fifo *dest);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NLUTILS_FIFO_H_ */
