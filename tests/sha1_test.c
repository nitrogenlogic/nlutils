/*
 * SHA-1 command-line test - extracted/derived from src/sha1.c (public domain)
 *
 * Modified Oct. 2011 by Mike Bourgeous
 */
#include <stdio.h>

#include "nlutils.h"

int main(int argc, char** argv)
{
	int i, j;
	struct nl_sha1_ctx context;
	unsigned char digest[SHA1_DIGEST_SIZE], buffer[16384];
	FILE* file;

	if (argc > 2) {
		puts("Public domain SHA-1 implementation - by Steve Reid <sreid@sea-to-sky.net>");
		puts("Modified for 16 bit environments 7/98 - by James H. Brown <jbrown@burgoyne.com>");	/* JHB */
		puts("Produces the SHA-1 hash of a file, or stdin if no file is specified.");
		return(0);
	}
	if (argc < 2) {
		file = stdin;
	}
	else {
		if (!(file = fopen(argv[1], "rb"))) {
			fputs("Unable to open file.", stderr);
			return(-1);
		}
	} 
	nl_sha1_init(&context);
	while (!feof(file)) {  /* note: what if ferror(file) */
		i = fread(buffer, 1, 16384, file);
		nl_sha1_update(&context, buffer, i);
	}
	nl_sha1_final(&context, digest);
	fclose(file);
	for (i = 0; i < SHA1_DIGEST_SIZE/4; i++) {
		for (j = 0; j < 4; j++) {
			printf("%02x", digest[i*4+j]);
		}
	}
	putchar('\n');
	return(0);	/* JHB */
}
