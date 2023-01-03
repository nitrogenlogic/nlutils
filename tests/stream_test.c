/*
 * Tests stream/file I/O utility functions.
 * Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <fcntl.h>

#include "nlutils.h"

// Pass argv[0] into exe
int test_open_timeout(char *exe)
{
	struct timespec now = {0, 0}, after = {0, 0};
	int fd;

	INFO_OUT("Testing nl_open_timeout().\n");

	DEBUG_OUT("Verifying successful opens occur quickly.\n");
#ifdef DEBUG
	// Things are slower in debug builds and in the CI pipeline
	nl_clock_fromnow(CLOCK_MONOTONIC, &after, (struct timespec){.tv_nsec = 150000000}); // 150ms
#else
	nl_clock_fromnow(CLOCK_MONOTONIC, &after, (struct timespec){.tv_nsec = 50000000}); // 50ms
#endif /* DEBUG */

	fd = nl_open_timeout(exe, O_RDONLY, 0, (struct timespec){.tv_sec = 1, .tv_nsec = 0});
	clock_gettime(CLOCK_MONOTONIC, &now);

	if(fd < 0) {
		ERROR_OUT("Opening %s failed when it should have succeeded: %s\n", exe, strerror(-fd));
		return -1;
	}

	close(fd);

	if(NL_TIMESPEC_GTE(now, after)) {
		after = nl_sub_timespec(now, after);
		after = nl_add_timespec(after, (struct timespec){.tv_nsec = 5000000});
		ERROR_OUT("A successful open took too long (%ld.%09ld elapsed)\n",
				(long)after.tv_sec, (long)after.tv_nsec);
		return -1;
	}

	DEBUG_OUT("Verifying failed opens take the full timeout\n");
	nl_clock_fromnow(CLOCK_MONOTONIC, &after, (struct timespec){.tv_sec = 1});
	fd = nl_open_timeout("/", O_RDWR | O_EXCL, 0, (struct timespec){.tv_sec = 1});
	clock_gettime(CLOCK_MONOTONIC, &now);

	if(fd >= 0) {
		ERROR_OUT("Expected opening / in O_EXCL to fail.\n");
		return -1;
	}

	if(NL_TIMESPEC_GTE(after, now)) {
		after = nl_sub_timespec(after, (struct timespec){.tv_sec = 1});
		after = nl_sub_timespec(now, after);
		ERROR_OUT("Expected failed opening of / to take longer (%ld.%09ld elapsed).\n",
				(long)after.tv_sec, (long)after.tv_nsec);
		return -1;
	}

	return 0;
}

int test_read_file(char *input_file)
{
	int ret = 0;

	INFO_OUT("Testing nl_read_file(%s).\n", input_file);

	struct nl_raw_data *data = nl_read_file(input_file);
	if (data == NULL) {
		ERROR_OUT("Failed to read %s.\n", input_file);
		return -1;
	}

	if (data->size == 0) {
		ERROR_OUT("Reading %s returned a size of zero.\n", input_file);
		ret = -1;
	}

	if (data->data == NULL) {
		ERROR_OUT("Reading %s returned a NULL internal data pointer.\n", input_file);
		ret = -1;
	}

	if (data->data != NULL && data->data[data->size] != 0) {
		ERROR_OUT("Returned data for %s was not terminated by a NUL byte.\n", input_file);
		ret = -1;
	}

	nl_destroy_data(data);

	return ret;
}

int main(int argc, char *argv[])
{
	int fail = 0;

	if(argc != 1) {
		return -1;
	}

	if(test_open_timeout(argv[0])) {
		fail += 1;
	}

	if (test_read_file(argv[0])) {
		fail += 1;
	}

	// TODO: Test other stream.c functions

	if (fail) {
		ERROR_OUT("%d stream tests failed.\n", fail);
	} else {
		INFO_OUT("All stream tests passed.\n");
	}

	return fail;
}
