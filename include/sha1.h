/* public api for steve reid's public domain SHA-1 implementation */
/* this file is in the public domain */
/* See sha1.c for a list of modifications made over time */
// http://www.koders.com/c/fidC6567C459CD4C9B4692F8CC8E0258843ED74E307.aspx

// Modified by Mike Bourgeous to namespace under nlutils and add nl_sha1() wrapper

#ifndef NL_SHA1_H
#define NL_SHA1_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct nl_sha1_ctx {
	uint32_t state[5];
	uint32_t count[2];
	uint8_t  buffer[64];
};

#define SHA1_DIGEST_SIZE 20

void nl_sha1_init(struct nl_sha1_ctx* context);
void nl_sha1_update(struct nl_sha1_ctx* context, const uint8_t* data, const size_t len);
void nl_sha1_final(struct nl_sha1_ctx* context, uint8_t digest[SHA1_DIGEST_SIZE]);

/*
 * Calculates and returns the hexadecimal SHA-1 hash of the given data.  The
 * returned buffer must be free()d.  Returns NULL on error.  If this data is
 * sensitive (e.g. a password), the memory storing it should be memset() to
 * zero after completion to avoid information leakage.
 */
char *nl_sha1(uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* NL_SHA1_H */
