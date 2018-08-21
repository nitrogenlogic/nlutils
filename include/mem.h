/*
 * Memory management functions.
 * Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_MEM_H_
#define NLUTILS_MEM_H_

/*
 * A hybrid between calloc() and realloc().  Uses realloc() to allocate or
 * resize the memory block at ptr from elem_size * old_count to elem_size *
 * count, clearing any newly added space to 0.  Does not overwrite the old
 * pointer if reallocation fails.  Does not perform any checking of parameters,
 * so take care to use consistent size and count values.
 *
 * If count is 0, free() is called on *ptr and NULL is returned.  If *ptr is
 * NULL, a new memory block will be allocated by realloc(), unless count is 0.
 *
 * Returns the new pointer and stores it in *ptr on success.  Returns NULL on
 * error, leaving *ptr and the errno value set by realloc() untouched.
 */
void *nl_crealloc(void **ptr, size_t elem_size, size_t old_count, size_t count);

#endif /* NLUTILS_MEM_H_ */
