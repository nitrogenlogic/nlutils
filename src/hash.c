/*
 * A naïve implementation of an associative array with string keys and values.
 * Copied from the logic system's depthcam/zonevar plugin.
 * Copyright (C)2011, 2014 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include "nlutils.h"

// TODO: Use an actual hash table, probably a public domain or BSD version.
// TODO: Add support for storing nl_variant or void* instead of char*.

/*
 * Returns a matching entry, if any, or NULL.  TODO: This is O(n).  Does not
 * check for NULL parameters.
 */
struct nl_hash_entry *nl_hash_find(const struct nl_hash * const hash, const char * const key)
{
	const struct nl_fifo_element *iter = NULL;
	struct nl_hash_entry *entry;

	while((entry = nl_fifo_next(hash->table, &iter))) {
		if(!strcmp(entry->key, key)) {
			return entry;
		}
	}

	if(iter == nl_fifo_error) {
		ERROR_OUT("Error iterating over key-value pairs.\n");
	}

	return NULL;
}

/*
 * Retrieves the given key's value from the given hash, or NULL if the key does
 * not exist or an error occurred.
 */
const char *nl_hash_get(const struct nl_hash * const hash, const char * const key)
{
	struct nl_hash_entry *entry;

	if(CHECK_NULL(hash) || CHECK_NULL(key)) {
		return NULL;
	}

	entry = nl_hash_find(hash, key);
	if(entry) {
		return entry->value;
	}

	return NULL;
}

// To be used only by nl_hash_set()
static int nl_hash_add(struct nl_hash *hash, char *key, char *value)
{
	struct nl_hash_entry *entry;

	entry = malloc(sizeof(struct nl_hash_entry));
	if(entry == NULL) {
		ERRNO_OUT("Error allocating memory for new hash entry");
		return -1;
	}

	entry->key = nl_strdup(key);
	if(entry->key == NULL) {
		ERROR_OUT("Error duplicating key for new hash entry.\n");
		free(entry);
		return -1;
	}

	entry->value = nl_strdup(value);
	if(entry->value == NULL) {
		ERROR_OUT("Error duplicating value for new hash entry.\n");
		free(entry->key);
		free(entry);
		return -1;
	}

	if(nl_fifo_put(hash->table, entry) < 0) {
		ERROR_OUT("Error adding hash entry to table.\n");
		free(entry->value);
		free(entry->key);
		free(entry);
		return -1;
	}

	hash->count++;

	return 0;
}

/*
 * Sets the given key to the given value, adding a new entry to the table if
 * the key is not found, replacing the existing value in the table if the key
 * is already present.  Returns 0 on success, -1 on error.
 */
int nl_hash_set(struct nl_hash *hash, char *key, char *value)
{
	struct nl_hash_entry *entry;
	char *oldval, *newval;

	if(CHECK_NULL(hash) || CHECK_NULL(key) || CHECK_NULL(value)) {
		return -1;
	}

	entry = nl_hash_find(hash, key);
	if(entry != NULL) {
		newval = nl_strdup(value);
		if(newval == NULL) {
			ERROR_OUT("Error duplicating new value for existing hash entry.\n");
			return -1;
		}
		oldval = entry->value;
		entry->value = newval;
		free(oldval);
	} else {
		return nl_hash_add(hash, key, value);
	}

	return 0;
}

// Internal use by nl_hash_remove() and nl_hash_destroy()
static void nl_hash_destroy_entry(struct nl_hash_entry *entry)
{
	free(entry->key);
	free(entry->value);
	free(entry);
}

/*
 * Removes the given key from the given hash table, if it exists.  Returns 0 on
 * success (including if the key did not exist), -1 on error.
 */
int nl_hash_remove(struct nl_hash *hash, char *key)
{
	struct nl_hash_entry *entry;

	if(CHECK_NULL(hash) || CHECK_NULL(key)) {
		return -1;
	}

	entry = nl_hash_find(hash, key);
	if(entry) {
		if(nl_fifo_remove(hash->table, entry)) {
			ERROR_OUT("Error removing entry from hash table.\n");
			return -1;
		}
		nl_hash_destroy_entry(entry);
		hash->count--;
	}

	return 0;
}

/*
 * Iterates over all entries in the given hash.  Do not modify the hash table
 * from within the callback.
 */
void nl_hash_iterate(const struct nl_hash * const hash, nl_hash_callback callback, void *cb_data)
{
	struct nl_hash_entry *entry;
	const struct nl_fifo_element *iter = NULL;

	if(CHECK_NULL(hash) || CHECK_NULL(callback)) {
		return;
	}

	while((entry = nl_fifo_next(hash->table, &iter))) {
		if(callback(cb_data, entry->key, entry->value)) {
			break;
		}
	}
}

/*
 * Creates a new hash table.  Returns NULL on error.  Hash table functions are
 * not thread safe.
 */
struct nl_hash *nl_hash_create()
{
	struct nl_hash *hash;

	hash = malloc(sizeof(struct nl_hash));
	if(hash == NULL) {
		ERRNO_OUT("Error allocating memory for new hash");
		return NULL;
	}

	hash->count = 0;
	hash->table = nl_fifo_create();
	if(hash->table == NULL) {
		ERROR_OUT("Error creating table to store hash entries.\n");
		free(hash);
		return NULL;
	}

	return hash;
}

// Callback used by nl_hash_clone() to insert keys into the cloned hash.
static int hash_clone_callback(void *cb_data, char *key, char *value)
{
	return nl_hash_set((struct nl_hash *)cb_data, key, value);
}

/*
 * Creates a deep copy (key and value strings are cloned as well as the entry
 * struct) of the given hash table.  Returns NULL on error.
 */
struct nl_hash *nl_hash_clone(const struct nl_hash * const hash)
{
	struct nl_hash *new_hash;

	if(CHECK_NULL(hash)) {
		return NULL;
	}

	new_hash = nl_hash_create();
	if(new_hash == NULL) {
		ERROR_OUT("Error creating new hash table for clone.\n");
		return NULL;
	}

	nl_hash_iterate(hash, hash_clone_callback, new_hash);

	if(hash->count != new_hash->count) {
		ERROR_OUT("Error adding entries to the cloned hash table.  Expected %zu, found %zd.\n",
				hash->count, new_hash->count);
		nl_hash_destroy(new_hash);
		return NULL;
	}

	return new_hash;
}

/*
 * Removes all entries from the given hash table.
 */
void nl_hash_clear(struct nl_hash *hash)
{
	struct nl_hash_entry *entry;

	if(CHECK_NULL(hash)) {
		return;
	}

	while((entry = nl_fifo_get(hash->table))) {
		nl_hash_destroy_entry(entry);
	}

	hash->count = 0;
}

/*
 * Destroys the given hash table.
 */
void nl_hash_destroy(struct nl_hash *hash)
{
	if(CHECK_NULL(hash)) {
		return;
	}

	nl_hash_clear(hash);
	nl_fifo_destroy(hash->table);
	free(hash);
}
