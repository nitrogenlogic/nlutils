/*
 * File and stream utility functions.
 * Copyright (C)2014 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "nlutils.h"

/*
 * Reads from src until feof(src) returns nonzero, writing all data read to
 * destfd.  Use fseek()/ftell() to copy file contents multiple times.  Returns
 * the number of bytes written on success, -1 on error.
 */
ssize_t nl_stream_to_fd(FILE *src, int destfd)
{
	char buf[32768];
	ssize_t size = 0;
	size_t bytes;

	while(!feof(src)) {
		bytes = fread(buf, 1, sizeof(buf), src);
		if(bytes == 0 && ferror(src)) {
			ERRNO_OUT("nl_stream_to_fd(): Error reading from source file");
			return -1;
		}

		if(write(destfd, buf, bytes) != (ssize_t)bytes) {
			ERRNO_OUT("nl_stream_to_fd(): Error writing to destination fd %d", destfd);
			return -1;
		}

		size += bytes;
	}

	return size;
}

/*
 * Writes the given data to fd.  Returns 0 on success, -1 if an error occurs.
 */
int nl_write_stream(int fd, const struct nl_raw_data * const data)
{
	ssize_t ret;
	size_t off = 0;

	if(CHECK_NULL(data) || CHECK_NULL(data->data)) {
		return -1;
	}

	if(fd < 0) {
		ERROR_OUT("Invalid file descriptor %d\n", fd);
		return -1;
	}

	do {
		ret = write(fd, data->data + off, data->size - off);
		if(ret < 0) {
			ERRNO_OUT("Error writing data to fd %d", fd);
			return -1;
		}

		if(ret > 0) {
			off += ret;
		}
	} while(ret > 0 && off < data->size);

	if(off < data->size) {
		ERROR_OUT("Only wrote %zu of %zu bytes of data to fd %d.\n", off, data->size, fd);
		return -1;
	}

	return 0;
}

/*
 * Reads from fd until end-of-file occurs, or until the end of available data
 * if O_NONBLOCK is set.  For added safety, a terminating NUL byte is appended
 * to the data but is not counted in the returned size.  Call nl_destroy_data()
 * on the returned pointer.  The stream is not closed after reading.  Returns
 * the data (includes size and contents) on success, NULL on error.
 */
struct nl_raw_data *nl_read_stream(int fd)
{
	struct nl_raw_data *data;
	char readbuf[16384];
	char *tmpbuf;
	ssize_t ret;

	if(fd < 0) {
		ERROR_OUT("Invalid file descriptor %d\n", fd);
		return NULL;
	}

	data = calloc(1, sizeof(struct nl_raw_data));
	if(data == NULL) {
		ERRNO_OUT("Error allocating data+length structure");
		return NULL;
	}

	do {
		errno = 0;
		ret = read(fd, readbuf, sizeof(readbuf));
		if(ret > 0) {
			tmpbuf = realloc(data->data, data->size + ret + 1);
			if(tmpbuf == NULL) {
				ERRNO_OUT("Error growing read buffer");
				goto error;
			}
			data->data = tmpbuf;

			memmove(data->data + data->size, readbuf, ret);
			data->size += ret;
			data->data[data->size] = 0;
		}
	} while(ret > 0);

	if(errno && errno != EAGAIN && errno != EWOULDBLOCK) {
		ERRNO_OUT("Error reading fd %d into a buffer", fd);
		goto error;
	}

	return data;

error:
	if(data != NULL) {
		nl_destroy_data(data);
	}

	return NULL;
}

/*
 * Reads the entire contents of a given filename into a struct nl_raw_data.
 * Call nl_destroy_data() (not free!) to free the struct.  Returns NULL if
 * there was an error.
 */
struct nl_raw_data *nl_read_file(const char *filename)
{
	int fd = open(filename, O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		ERRNO_OUT("Error opening %s for reading", filename);
		return NULL;
	}

	struct nl_raw_data *data = nl_read_stream(fd);
	if (data == NULL) {
		ERROR_OUT("Error reading contents of %s\n", filename);
		close(fd);
		return NULL;
	}

	close(fd);

	return data;
}

/*
 * Sets the FD_CLOEXEC flag on the given file descriptor.  Returns 0 on
 * success, -1 on error
 */
int nl_set_cloexec(int fd)
{
	if(fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC) == -1) {
		ERRNO_OUT("Error setting FD_CLOEXEC flag on fd %d", fd);
		return -1;
	}

	return 0;
}

/*
 * Sets (nonblock is nonzero) or clears (nonblock is zero) the O_NONBLOCK flag
 * on the given file descriptor.  Returns 0 on success, -1 on error.
 */
int nl_set_nonblock(int fd, int nonblock)
{
	int flags = fcntl(fd, F_GETFL);

	if(flags == -1) {
		ERRNO_OUT("Error reading current flags on fd %d", fd);
		return -1;
	}

	if(nonblock) {
		flags |= O_NONBLOCK;
	} else {
		flags &= ~O_NONBLOCK;
	}

	if(fcntl(fd, F_SETFL, flags)) {
		ERRNO_OUT("Error setting/clearing nonblock flag on fd %d", fd);
		return -1;
	}

	return 0;
}

/*
 * Repeatedly tries to open the given file with open() and the given flags,
 * until the given timeout (relative to CLOCK_MONOTONIC) has elapsed.  Returns
 * the opened file descriptor on success, or a negative errno-like value on
 * error.  Returns -ETIMEDOUT if the timeout expires with no other error.
 *
 * Example:
 * 	int fd = nl_open_timeout('/tmp/file', O_WRONLY | O_CREAT, 0600,
 * 		(struct timespec){.tv_sec = 1, .tv_nsec = 0});
 */
int nl_open_timeout(char *pathname, int flags, mode_t mode, struct timespec timeout)
{
	struct timespec done = {0, 0};
	struct timespec now = {0, 0};
	int ret, old_err;

	ret = nl_clock_fromnow(CLOCK_MONOTONIC, &done, timeout);
	if(ret) {
		ERROR_OUT("Error getting time interval for open timeout: %s\n", strerror(ret));
		return -ret;
	}

	do {
		ret = open(pathname, flags | O_NONBLOCK, mode);
		if(ret == -1) {
			// TODO: Abstract "if time elapsed, then code" pattern
			old_err = errno;
			if(clock_gettime(CLOCK_MONOTONIC, &now)) {
				old_err = errno;
				ERRNO_OUT("Error getting current time for open timeout");
				return -old_err;
			}

			if(NL_TIMESPEC_GTE(now, done)) {
				if(old_err == EAGAIN || old_err == EWOULDBLOCK) {
					return -ETIMEDOUT;
				}

				return -old_err;
			}

			nl_usleep(10000);
		}
	} while(ret == -1);

	// Clear O_NONBLOCK if it wasn't set in original flags
	if(nl_set_nonblock(ret, flags & O_NONBLOCK)) {
		close(ret);
		return -EIO;
	}

	return ret;
}
