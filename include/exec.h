/*
 * Process execution utility functions.
 * Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_EXEC_H_
#define NLUTILS_EXEC_H_

#include <sys/types.h>

/*
 * Runs the given command (without searching $PATH) with the given arguments in
 * another process, with full remote interaction capabilities.  The arguments
 * must be passed as specified by execve(), ending with a NULL array entry.
 * Writing to *writefd will write to the command's stdin.  Reading from *readfd
 * will read from the command's stdout.  Reading from *errfd will read from the
 * command's stderr.  If NULL is passed for writefd, readfd, or errfd, then the
 * command's stdin, stdout, or stderr will point to those of the current
 * process.  Returns the child PID on success, -1 on error.  The caller should
 * use nl_wait_get_return() (or similar) and close the file descriptors when
 * done.
 *
 * Note that the called process will inherit open file descriptors, so make
 * sure that opened files are set to close (e.g. with nl_set_cloexec()).  The
 * file descriptors returned by this function will already be set to close on
 * execute, but may still be inherited if another thread executes another
 * process between creation and marking as FD_CLOEXEC.
 */
pid_t nl_popen3ve(int *writefd, int *readfd, int *errfd, const char *cmd, char *const argv[], char *const envp[]);

/*
 * Exactly like nl_popen3ve().  If callback is not NULL, it will be called in
 * the child process after forking.  The callback will be called before the
 * child process's standard I/O is redirected to pipes.  An example use of the
 * callback is dropping privileges before the command executes.
 */
pid_t nl_popen3vec(int *writefd, int *readfd, int *errfd, const char *cmd, char *const argv[], char *const envp[], void (*callback)(void));

/*
 * Runs command in another process with full remote interaction capabilities.
 * Be aware that command is passed to sh -c, so shell expansion will occur.
 * Writing to *writefd will write to the command's stdin.  Reading from *readfd
 * will read from the command's stdout.  Reading from *errfd will read from the
 * command's stderr.  If NULL is passed for writefd, readfd, or errfd, then the
 * command's stdin, stdout, or stderr will not be changed.  Returns the child
 * PID on success, -1 on error.
 */
pid_t nl_popen3(char *command, int *writefd, int *readfd, int *errfd);

/*
 * Runs the command (without searching PATH) with the given argument list and
 * environment (in the format used by execve()), reading its stdout output into
 * an nl_raw_data structure that must be destroyed with nl_destroy_data().  If
 * *output is not NULL, it will be written to the command's stdin before
 * reading from the command's stdout.  Returns NULL on error.
 */
struct nl_raw_data *nl_popenve_readall(const char *cmd, char *const argv[], char *const envp[], struct nl_raw_data *output);

/*
 * Runs the given command (with shell expansion, via nl_popen3()), reading all
 * its stdout output into a buffer allocated with malloc()/realloc().  Once the
 * command has terminated, the buffer will be returned, and if size is not
 * NULL, the number of bytes read (excluding a terminating NUL added by this
 * function) will be stored in *size.  The returned string will be terminated
 * by NUL, but note that if the input data contains any NUL bytes the only way
 * to know the true length of the data is the *size parameter.  The returned
 * buffer must be free()d.  Returns NULL on error.
 */
char *nl_popen_readall(char *command, size_t *size);

/*
 * Waits for the given child PID to exit.  Returns the child's exit status
 * value if the child exited normally.  Returns a negative signal number, minus
 * 100, if the child was terminated by a signal (e.g. SIGINT would be -102).
 * Returns -1 on error, or if the child was stopped or continued.
 */
int nl_wait_get_return(pid_t pid);

#endif /* NLUTILS_EXEC_H_ */
