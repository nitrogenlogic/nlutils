/*
 * File and stream utility functions.
 * Copyright (C)2014 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_STREAM_H_
#define NLUTILS_STREAM_H_

/*
 * Reads from src until feof(src) returns nonzero, writing all data read to
 * destfd.  Use fseek()/ftell() to copy file contents multiple times.  Returns
 * the number of bytes written on success, -1 on error.
 */
ssize_t nl_stream_to_fd(FILE *src, int destfd);

/*
 * Writes the given data to fd.  Returns 0 on success, -1 if an error occurs.
 */
int nl_write_stream(int fd, const struct nl_raw_data * const data);

/*
 * Reads from fd until end-of-file occurs, or until the end of available data
 * if O_NONBLOCK is set.  For added safety, a terminating NUL byte is appended
 * to the data but is not counted in the returned size.  Call nl_destroy_data()
 * on the returned pointer.  The stream is not closed after reading.  Returns
 * the data (includes size and contents) on success, NULL on error.
 */
struct nl_raw_data *nl_read_stream(int fd);

/*
 * Sets the FD_CLOEXEC flag on the given file descriptor.  Returns 0 on
 * success, -1 on error
 */
int nl_set_cloexec(int fd);

/*
 * Sets (nonblock is nonzero) or clears (nonblock is zero) the O_NONBLOCK flag
 * on the given file descriptor.  Returns 0 on success, -1 on error.
 */
int nl_set_nonblock(int fd, int nonblock);

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
int nl_open_timeout(char *pathname, int flags, mode_t mode, struct timespec timeout);

#endif /* NLUTILS_STREAM_H_ */
