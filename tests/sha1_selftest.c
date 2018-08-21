/*
 * SHA-1 self test - extracted/derived from src/sha1.c (public domain)
 *
 * Modified Oct. 2011 by Mike Bourgeous for nlutils namespace/conventions,
 * lowercase unspaced digests, etc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nlutils.h"

static char *test_data[] = {
	"",
	"abc",
	"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
	"A million repetitions of 'a'"};
static char *test_results[] = {
	"da39a3ee5e6b4b0d3255bfef95601890afd80709",
	"a9993e364706816aba3e25717850c26c9cd0d89d",
	"84983e441c3bd26ebaae4aa1f95129e5e54670f1",
	"34aa973cd4c4daa4f61eeb2bdbad27316534016f"};


static void digest_to_hex(const uint8_t digest[SHA1_DIGEST_SIZE], char *output)
{
	int i,j;
	char *c = output;

	for (i = 0; i < SHA1_DIGEST_SIZE/4; i++) {
		for (j = 0; j < 4; j++) {
			sprintf(c,"%02x", digest[i*4+j]);
			c += 2;
		}
	}
}

int main()
{
	size_t k;
	struct nl_sha1_ctx context;
	uint8_t digest[20];
	char output[80];
	char *output2;

	fprintf(stdout, "verifying SHA-1 implementation... ");

	for (k = 0; k < ARRAY_SIZE(test_results) - 1; k++){ 
		nl_sha1_init(&context);
		nl_sha1_update(&context, (uint8_t*)test_data[k], strlen(test_data[k]));
		nl_sha1_final(&context, digest);
		digest_to_hex(digest, output);

		if (strcmp(output, test_results[k])) {
			fprintf(stdout, "FAIL\n");
			fprintf(stderr,"* hash of \"%s\" incorrect:\n", test_data[k]);
			fprintf(stderr,"\t%s returned\n", output);
			fprintf(stderr,"\t%s is correct\n", test_results[k]);
			return -1;
		}

		output2 = nl_sha1((uint8_t *)test_data[k], strlen(test_data[k]));
		if(output2 == NULL) {
			fprintf(stdout, "nl_sha1() FAIL\n");
			ERROR_OUT("nl_sha1() returned NULL.\n");
			free(output2);
			return -1;
		}
		if(strcmp(output2, test_results[k])) {
			fprintf(stdout, "nl_sha1() hash FAIL\n");
			fprintf(stderr,"* hash of \"%s\" by nl_sha1() incorrect:\n", test_data[k]);
			fprintf(stderr,"\t%s returned\n", output2);
			fprintf(stderr,"\t%s is correct\n", test_results[k]);
			free(output2);
			return -1;
		}
		free(output2);
	}
	/* million 'a' vector we feed separately */
	nl_sha1_init(&context);
	for (k = 0; k < 1000000; k++)
		nl_sha1_update(&context, (uint8_t*)"a", 1);
	nl_sha1_final(&context, digest);
	digest_to_hex(digest, output);
	if (strcmp(output, test_results[ARRAY_SIZE(test_results) - 1])) {
		fprintf(stdout, "FAIL\n");
		fprintf(stderr,"* hash of \"%s\" incorrect:\n", test_data[ARRAY_SIZE(test_results) - 1]);
		fprintf(stderr,"\t%s returned\n", output);
		fprintf(stderr,"\t%s is correct\n", test_results[ARRAY_SIZE(test_results) - 1]);
		return -1;
	}

	/* success */
	fprintf(stdout, "ok\n");
	return 0;
}
