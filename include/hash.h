/*
 * A naïve implementation of an associative array with string keys and values.
 * Copied and modified from the Automation Controller's depthcam/zonevar plugin.
 * Copyright (C)2011-2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_HASH_H
#define NLUTILS_HASH_H

// FIXME: It would be better to use an existing hash table implementation.
// Note that this is not yet a hash table; it's just an array.
struct nl_hash_entry {
	uint32_t hash;
	char *key;
	char *value;
	// TODO: Consider using a variant for the value
};
struct nl_hash {
	// TODO: Use an actual hash table, with hashing
	struct nl_fifo *table;
	size_t count;
};

/*
 * Callback for use by nl_hash_iterate().  Return nonzero to halt iteration.
 */
typedef int (*nl_hash_callback)(void *cb_data, char *key, char *value);


/*
 * Returns a matching entry, if any, or NULL.  TODO: This is O(n).  Does not
 * check for NULL parameters.
 */
struct nl_hash_entry *nl_hash_find(const struct nl_hash * const hash, const char * const key);

/*
 * Retrieves the given key's value from the given hash, or NULL if the key does
 * not exist or an error occurred.
 */
const char *nl_hash_get(const struct nl_hash * const hash, const char * const key);

/*
 * Sets the given key to the given value, adding a new entry to the table if
 * the key is not found, replacing the existing value in the table if the key
 * is already present.  Returns 0 on success, -1 on error.
 */
int nl_hash_set(struct nl_hash *hash, char *key, char *value);

/*
 * Removes the given key from the given hash table, if it exists.  Returns 0 on
 * success (including if the key did not exist), -1 on error.
 */
int nl_hash_remove(struct nl_hash *hash, char *key);

/*
 * Iterates over all entries in the given hash.  Do not modify the hash table
 * from within the callback.
 */
void nl_hash_iterate(const struct nl_hash * const hash, nl_hash_callback callback, void *cb_data);

/*
 * Creates a new hash table.  Returns NULL on error.  Hash table functions are
 * not thread safe.
 */
struct nl_hash *nl_hash_create();

/*
 * Creates a deep copy (key and value strings are cloned as well as the entry
 * struct) of the given hash table.  Returns NULL on error.
 */
struct nl_hash *nl_hash_clone(const struct nl_hash * const hash);

/*
 * Removes all entries from the given hash table.
 */
void nl_hash_clear(struct nl_hash *hash);

/*
 * Destroys the given hash table.
 */
void nl_hash_destroy(struct nl_hash *hash);

#endif /* NLUTILS_HASH_H_ */
