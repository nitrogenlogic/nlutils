/*
 * Extracts a Nitrogen Logic firmware image.
 * Copyright (C)2011-2018 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif /* _XOPEN_SOURCE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "nlutils.h"


/*
 * Calculates the sha1sum of the gzip decompressed remainder of the given
 * stream.  The csum array must be able to hold at least 128 bytes.  The stream
 * is rewound to the position it had at the start of this function.  Returns
 * the compressed size of the data on success, -1 on error.
 */
static ssize_t checksum(FILE *f, char *csum)
{
	int wrfd, rdfd;
	FILE *rd;
	ssize_t size;
	off_t start;
	int ret;

	start = ftello(f);
	if(start == -1) {
		perror("Error getting current file position");
		return -1;
	}

	pid_t pid = nl_popen3("gzip -d | sha1sum 2>/dev/null", &wrfd, &rdfd, NULL);
	if(pid == -1) {
		fprintf(stderr, "Unable to start checksum verification process.\n");
		return -1;
	}

	// Write data to the stdin pipe
	size = nl_stream_to_fd(f, wrfd);
	if(size == -1) {
		fprintf(stderr, "Error copying data to checksum process.\n");
		close(rdfd);
		close(wrfd);
		kill(pid, SIGKILL);
		return -1;
	}
	close(wrfd);

	// Wait for the process to finish
	ret = nl_wait_get_return(pid);
	if(ret) {
		if(ret == -1) {
			fprintf(stderr, "Error waiting for checksum process to finish.\n");
		} else {
			fprintf(stderr, "Checksum process failed with return code %d.\n", ret);
		}
		close(rdfd);
		return -1;
	}

	// Read the result from the stdout pipe
	rd = fdopen(rdfd, "rb");
	if(rd == NULL) {
		perror("Error opening checksum file descriptor as stream");
		close(rdfd);
		return -1;
	}
	if(fscanf(rd, "%127[0-9A-Fa-f]%*2[ ]-%*1[\n]", csum) != 1) {
		fprintf(stderr, "Error reading computed checksum.\n");
		fclose(rd);
		return -1;
	}
	fclose(rd);

	if(fseeko(f, start, SEEK_SET) == -1) {
		perror("Error rewinding stream after calculating checksum");
	}

	return size;
}

/*
 * Decrypts, decompresses, and extracts the remainder of the given stream as a
 * firmware image.  Returns the compressed size of the data on success, -1 on
 * error.
 */
static ssize_t extract(FILE *f)
{
	int wrfd;
	ssize_t size;
	int ret;

	pid_t pid = nl_popen3("gzip -d | bash", &wrfd, NULL, NULL);
	if(pid == -1) {
		fprintf(stderr, "Unable to start extraction process.\n");
		return -1;
	}

	// Write data to the stdin pipe
	size = nl_stream_to_fd(f, wrfd);
	if(size == -1) {
		fprintf(stderr, "Error copying data to extraction process.\n");
		close(wrfd);
		kill(pid, SIGKILL);
		return -1;
	}
	close(wrfd);

	// Wait for the process to finish
	ret = nl_wait_get_return(pid);
	if(ret) {
		if(ret == -1) {
			fprintf(stderr, "Error waiting for extraction process to finish.\n");
		} else {
			fprintf(stderr, "Extraction process failed with return code %d.\n", ret);
		}
		return -1;
	}

	return size;
}

int main(int argc, char *argv[])
{
	struct stat statbuf;
	char csum_in[128] = "";
	char csum_verify[128] = "";
	char arch_line[64] = "";
	char name_line[64] = "";
	FILE *fw;
	int version;
	ssize_t exec_size = 0;

	if(argc != 2) {
		printf("Usage: %s (firmware_file|--version)\n", argv[0]);
		return -1;
	}

	if(!strcmp(argv[1], "--version")) {
		printf("%s\n", NLUTILS_VERSION);
		return 0;
	}

	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		perror("Unable to ignore SIGPIPE");
		return -1;
	}

	if(stat(argv[1], &statbuf)) {
		perror("Unable to get firmware file information");
		return -1;
	}

	fw = fopen(argv[1], "rb");
	if(fw == NULL) {
		perror("Unable to open firmware file for reading");
		return -1;
	}

	if(fscanf(fw, "NLFW_%02d%*1[\n]", &version) != 1) {
		fprintf(stderr, "This does not appear to be a valid firmware file (firmware file type identifier not found).\n");
		goto error;
	}

	if(version != 3) {
		fprintf(stderr, "Incompatible firmware file version %d.\n", version);
		goto error;
	}

	int c;

	// Check architecture
	c = getc(fw);
	if(c != '\n') {
		if(c == EOF || ungetc(c, fw) != c || fscanf(fw, "%63[^\n]%*1[\n]", arch_line) != 1) {
			fprintf(stderr, "Architecture line not found.\n");
			goto error;
		}
		if(strlen(arch_line)) {
			char *arch = nl_popen_readall("uname -m | tr -d '\\r\\n'", NULL);
			if(arch == NULL || strlen(arch) == 0) {
				fprintf(stderr, "Error getting local architecture.\n");
				free(arch);
				goto error;
			}

			if(strcmp(arch, arch_line)) {
				fprintf(stderr, "Architecture mismatch.  Firmware is for '%s', this is '%s'.\n",
						arch_line, arch);
				free(arch);
				goto error;
			}

			free(arch);
		}
	}

	// Print firmware name
	c = getc(fw);
	if(c != '\n') {
		if(c == EOF || ungetc(c, fw) != c || fscanf(fw, "%63[^\n]%*1[\n]", name_line) != 1) {
			fprintf(stderr, "Error reading firmware name.\n");
			goto error;
		}
		if(strlen(name_line)) {
			printf("Firmware name: '%s'\n", name_line);
		}
	}

	// Read checksum
	if(fscanf(fw, "%40[0-9a-f]%*1[\n]", csum_in) != 1) {
		fprintf(stderr, "Checksum not found.\n");
		goto error;
	}

	// Verify checksum (TODO: use nl_sha1*() and zlib)
	exec_size = checksum(fw, csum_verify);
	if(exec_size == -1) {
		fprintf(stderr, "Error calculating checksum for firmware file.\n");
		goto error;
	}
	if(strcmp(csum_in, csum_verify)) {
		fprintf(stderr, "Input checksum '%s' does not match verification checksum '%s'.\n",
				csum_in, csum_verify);
		goto error;
	}

	printf("%zu bytes of firmware data verified.\n", exec_size);
	fflush(stdout);

	// Extract data
	if(chdir("/")) {
		fprintf(stderr, "Error changing to root directory.\n");
		goto error;
	}
	if(extract(fw) == -1) {
		fprintf(stderr, "Error extracting firmware.\n");
		goto error;
	}

	fclose(fw);

	printf("Firmware extraction complete.\n");

	return 0;

error:
	fclose(fw);
	return -1;
}
