/*
 * Functions used for debugging purposes (e.g. printing backtraces).
 * Copyright (C)2013 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_DEBUG_H_
#define NLUTILS_DEBUG_H_

/*
 * Information about a symbol within a symbol table.
 */
struct sym_info {
	char name[256];
	void *addr;

	struct sym_info *next;
};

/*
 * Reads a symbol map from a file whose name is the name of the target of the
 * /proc/self/exe link with ".map" appended.  Returns the number of symbols
 * read on success, or a negated errno constant on error.  This function is not
 * reentrant.
 */
int load_symbols();

/*
 * Deallocates all memory used by any symbol map previously loaded using
 * load_symbols().
 */
void unload_symbols();

/*
 * Stores information about the given address into the given Dl_info structure.
 * First checks with dladdr(), then if no match is found, checks the loaded
 * symbol table.  Returns zero on success, -1 if no match was found.  Does not
 * check for a NULL syminf pointer.
 */
int find_symbol(void *addr, Dl_info *syminf);

/*
 * Prints the given stack trace (using nl_fptmf() for timestamps).  The
 * backtrace may be generated by glibc's backtrace() method.  If a symbol map
 * has been loaded, it will be used to try to fill in symbol names that are
 * missed by dladdr().
 */
void print_backtrace(FILE *out, void **trace, int count);

/*
 * Returns a pointer to a string describing the given signal and .si_code from
 * siginfo_t.
 */
const char *strsigcode(int signum, int si_code);

/*
 *  Prints a backtrace of the current thread using glibc's backtrace() and our
 *  print_backtrace().
 */
#define PRINT_TRACE() do { \
	void *bk_trace[40]; \
	int bk_count; \
	bk_count = backtrace(bk_trace, ARRAY_SIZE(bk_trace)); \
	nl_fptmf(stderr, "%d backtrace elements:\n", bk_count); \
	print_backtrace(stderr, bk_trace, bk_count); \
} while(0)

#endif /* NLUTILS_DEBUG_H_ */