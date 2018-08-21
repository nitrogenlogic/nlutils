/*
 * Process execution utility functions.
 * Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "nlutils.h"


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
pid_t nl_popen3ve(int *writefd, int *readfd, int *errfd, const char *cmd, char *const argv[], char *const envp[])
{
	return nl_popen3vec(writefd, readfd, errfd, cmd, argv, envp, NULL);
}

/*
 * Exactly like nl_popen3ve().  If callback is not NULL, it will be called in
 * the child process after forking.  The callback will be called before the
 * child process's standard I/O is redirected to pipes.  An example use of the
 * callback is dropping privileges before the command executes.
 */
pid_t nl_popen3vec(int *writefd, int *readfd, int *errfd, const char *cmd, char *const argv[], char *const envp[], void (*callback)(void))
{
	int in_pipe[2] = {-1, -1};
	int out_pipe[2] = {-1, -1};
	int err_pipe[2] = {-1, -1};
	pid_t pid;

	if(CHECK_NULL(cmd) || CHECK_NULL(argv) || CHECK_NULL(envp)) {
		goto error;
	}

	if(access(cmd, X_OK | R_OK)) {
		ERRNO_OUT("Unable to execute external command");
		goto error;
	}

	if(writefd && pipe(in_pipe)) {
		ERRNO_OUT("Error creating pipe for stdin");
		goto error;
	}
	if(readfd && pipe(out_pipe)) {
		ERRNO_OUT("Error creating pipe for stdout");
		goto error;
	}
	if(errfd && pipe(err_pipe)) {
		ERRNO_OUT("Error creating pipe for stderr");
		goto error;
	}

	pid = fork();
	switch(pid) {
		case -1:
			// Error
			ERRNO_OUT("Error creating child process");
			goto error;

		case 0:
			// Child
			if(callback) {
				callback();
			}

			if(writefd) {
				close(in_pipe[1]);
				if(dup2(in_pipe[0], 0) == -1) {
					ERRNO_OUT("Error assigning stdin in child process");
					exit(-1);
				}
				close(in_pipe[0]);
			}
			if(readfd) {
				close(out_pipe[0]);
				if(dup2(out_pipe[1], 1) == -1) {
					ERRNO_OUT("Error assigning stdout in child process");
					exit(-1);
				}
				close(out_pipe[1]);
			}
			if(errfd) {
				close(err_pipe[0]);
				if(dup2(err_pipe[1], 2) == -1) {
					ERRNO_OUT("Error assigning stderr in child process");
					exit(-1);
				}
				close(err_pipe[1]);
			}

			execve(cmd, argv, envp);

			// TODO: Have the parent wait very briefly for failure
			// of execve, notify via a pipe, so parent can return
			// failure?

			ERRNO_OUT("Error executing command in child process");
			exit(-1);

		default:
			// Parent
			break;
	}

	if(writefd) {
		close(in_pipe[0]);
		nl_set_cloexec(in_pipe[1]);
		*writefd = in_pipe[1];
	}
	if(readfd) {
		close(out_pipe[1]);
		nl_set_cloexec(out_pipe[0]);
		*readfd = out_pipe[0];
	}
	if(errfd) {
		close(err_pipe[1]);
		nl_set_cloexec(out_pipe[0]);
		*errfd = err_pipe[0];
	}

	return pid;

error:
	if(in_pipe[0] >= 0) {
		close(in_pipe[0]);
	}
	if(in_pipe[1] >= 0) {
		close(in_pipe[1]);
	}
	if(out_pipe[0] >= 0) {
		close(out_pipe[0]);
	}
	if(out_pipe[1] >= 0) {
		close(out_pipe[1]);
	}
	if(err_pipe[0] >= 0) {
		close(err_pipe[0]);
	}
	if(err_pipe[1] >= 0) {
		close(err_pipe[1]);
	}

	return -1;
}

/*
 * Runs command in another process with full remote interaction capabilities.
 * Be aware that command is passed to sh -c, so shell expansion will occur.
 * Writing to *writefd will write to the command's stdin.  Reading from *readfd
 * will read from the command's stdout.  Reading from *errfd will read from the
 * command's stderr.  If NULL is passed for writefd, readfd, or errfd, then the
 * command's stdin, stdout, or stderr will not be changed.  Returns the child
 * PID on success, -1 on error.
 */
pid_t nl_popen3(char *command, int *writefd, int *readfd, int *errfd)
{
	if(CHECK_NULL(command)) {
		return -1;
	}

	return nl_popen3ve(writefd, readfd, errfd, "/bin/sh",
			(char *[]){"/bin/sh", "-c", command, NULL}, environ);
}

/*
 * Runs the command (without searching PATH) with the given argument list and
 * environment, reading its stdout output into an nl_raw_data structure that
 * must be destroyed with nl_destroy_data().  If *output is not NULL, it will
 * be written to the command's stdin before reading from the command's stdout.
 * Returns NULL on error.
 */
struct nl_raw_data *nl_popenve_readall(const char *cmd, char *const argv[], char *const envp[], struct nl_raw_data *output)
{
	struct nl_raw_data *data = NULL;
	int outfd = -1;
	int infd = -1;
	pid_t pid = -1;

	// TODO: Allow retrieving the stderr (maybe in another function)

	if(output != NULL && output->data == NULL) {
		ERROR_OUT("Output data specified with NULL output->data\n");
		return NULL;
	}

	pid = nl_popen3ve(output ? &outfd : NULL, &infd, NULL, cmd, argv, envp);
	if(pid == -1) {
		ERROR_OUT("Error executing command with nl_popen3ve().\n");
		return NULL;
	}

	if(output && nl_write_stream(outfd, output)) {
		ERROR_OUT("Error writing to command's stdin.\n");
		goto error;
	}

	if(outfd >= 0 && close(outfd)) {
		ERRNO_OUT("Error closing command stdin stream.\n");
		outfd = -1;
		goto error;
	}
	outfd = -1;

	data = nl_read_stream(infd);
	if(data == NULL) {
		ERROR_OUT("Error reading command output into memory.\n");
		goto error;
	}

	if(close(infd)) {
		ERRNO_OUT("Error closing command stdout stream.\n");
		infd = -1;
		goto error;
	}
	infd = -1;

	if(nl_wait_get_return(pid) == -1) {
		ERROR_OUT("Error waiting for command to finish.\n");
		pid = -1;
		goto error;
	}

	return data;

error:
	if(data != NULL) {
		nl_destroy_data(data);
	}

	if(infd >= 0) {
		close(infd);
	}

	if(outfd >= 0) {
		close(outfd);
	}

	if(pid > 0) {
		// Prevent zombie children
		kill(pid, SIGKILL);
		nl_wait_get_return(pid);
	}

	return NULL;
}

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
char *nl_popen_readall(char *command, size_t *size)
{
	struct nl_raw_data *data;
	char *outbuf;

	if(CHECK_NULL(command)) {
		return NULL;
	}

	data = nl_popenve_readall("/bin/sh", (char *[]){"/bin/sh", "-c", command, NULL}, environ, NULL);
	if(data == NULL) {
		return NULL;
	}

	outbuf = data->data;
	if(size != NULL) {
		*size = data->size;
	}

	free(data);

	return outbuf;
}

/*
 * Waits for the given child PID to exit.  Returns the child's exit status
 * value if the child exited normally.  Returns a negative signal number, minus
 * 100, if the child was terminated by a signal (e.g. SIGINT would be -102).
 * Returns -1 on error, or if the child was stopped or continued.
 */
int nl_wait_get_return(pid_t pid)
{
	int status;

	if(waitpid(pid, &status, 0) == -1) {
		ERRNO_OUT("Error getting return status of child process");
		return -1;
	}

	if(WIFEXITED(status)) {
		return WEXITSTATUS(status);
	}
	if(WIFSIGNALED(status)) {
		return -(WTERMSIG(status) + 100);
	}

	return -1;
}
