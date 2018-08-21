/*
 * Nitrogen Logic utility library.
 * Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_NLUTILS_H_
#define NLUTILS_NLUTILS_H_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


// Allow generating non-inline versions of inline functions using inline_defs.c
#ifndef NLUTILS_INLINE
# define NLUTILS_INLINE inline
#endif /* NLUTILS_INLINE */


/*
 * Copyright string.
 */
extern const char * const nlutils_copyright;

/*
 * Version string.
 */
extern const char * const nlutils_version;

/***** Utility definitions *****/

/*
 * Evaluates the size of the array.  Does not work for malloc()ed arrays.
 * References its parameter multiple times.
 */
#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))

#define ARRAY_BIT(array, bit) (((array)[(bit) / (sizeof((array)[0]) * 8)] >> ((bit) % (sizeof((array)[0]) * 8))) & 1)

/*
 * These reference the returned parameter multiple times, so don't use them
 * with code that has any side effects.
 */
#define MIN_NUM(x, y) ((x) < (y) ? (x) : (y))
#define MAX_NUM(x, y) ((x) > (y) ? (x) : (y))
#define CLAMP(min, max, x) (MAX_NUM((min), MIN_NUM((max), (x))))

/*
 * DEBUG_OUT behaves similarly to fprintf(stdout, ...), but adds file, line,
 * and function information, as well as a timestamp and the name of the current
 * thread.
 */
#ifdef DEBUG
# define DEBUG_NEWLINE() printf("\n")
# define DEBUG_OUT(...) { nl_ptmf("%s:%d: %s():\t", __FILE__, __LINE__, __func__); printf(__VA_ARGS__); }
# define DEBUG_OUT_EX(...) { printf(__VA_ARGS__); }
#else /* DEBUG */
# define DEBUG_NEWLINE()
# define DEBUG_OUT(...)
# define DEBUG_OUT_EX(...)
#endif /* DEBUG */

/*
 * Behaves similarly to printf(...), but adds file, line, and function
 * information, as well as a timestamp and the name of the current thread.
 */
#define INFO_OUT(...) {\
	nl_ptmf("%s:%d: %s():\t", __FILE__, __LINE__, __func__);\
	printf(__VA_ARGS__);\
}
#define INFO_OUT_EX(...) printf(__VA_ARGS__)

/*
 * Behaves similarly to fprintf(stderr, ...), but adds file, line, and function
 * information, as well as a timestamp and the name of the current thread.
 */
#define ERROR_OUT(...) {\
	nl_fptmf(stderr, "\e[0;1m%s:%d: %s():\t", __FILE__, __LINE__, __func__);\
	fprintf(stderr, __VA_ARGS__);\
	fprintf(stderr, "\e[0m");\
}
#define ERROR_OUT_EX(...) {\
	fprintf(stderr, "\e[0;1m");\
	fprintf(stderr, __VA_ARGS__);\
	fprintf(stderr, "\e[0m");\
}

/*
 * Behaves similarly to perror(), but supports printf formatting and prints
 * file, line, and function information, as well as a timestamp and the name of
 * the current thread.
 */
#define ERRNO_OUT(...) {\
	nl_fptmf(stderr, "\e[0;1m%s:%d: %s():\t", __FILE__, __LINE__, __func__);\
	fprintf(stderr, __VA_ARGS__);\
	fprintf(stderr, ": %d (%s)\e[0m\n", errno, strerror(errno));\
}

/*
 * Prints an error message (including the expression passed in o) and evaluates
 * to nonzero if o is NULL.  Evaluates to zero if o is not NULL.
 */
#define CHECK_NULL(o) ( (o) == NULL ? ( nl_fptmf(stderr, "\e[0;1m%s:%d: %s(): \"%s\" is null.\e[0m\n", \
				__FILE__, __LINE__, __func__, #o), 1 ) : 0 )

/*
 * Prints an error message including the expressions passed in if a != b.
 * Evaluates to zero if a equals b.
 */
#define CHECK_NOT_EQUAL(a, b) ( (a) != (b) ? ( nl_fptmf(stderr, "\e[0;1m%s:%d: %s(): \"%s\" != \"%s\".\e[0m\n", \
				__FILE__, __LINE__, __func__, #a, #b), 1 ) : 0 )

/*
 * Returns the given string if it is non-NULL, or "[null]" if it is NULL.
 * Note: dereferences the parameter twice.
 */
#define GUARD_NULL(str) ( (str) ? (str) : "[null]" )


/***** Include submodules ****/

#include "nl_time.h"
#include "mem.h"
#include "str.h"
#include "escape.h"
#include "sha1.h"
#include "exec.h"
#include "stream.h"
#include "net.h"
#include "log.h"
#include "thread.h"
#include "variant.h"
#include "hash.h"
#include "kvp.h"
#include "url.h"
#include "fifo.h"
#include "url_req.h"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NLUTILS_NLUTILS_H_ */
