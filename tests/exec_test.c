/*
 * Tests program execution functions (e.g. nl_popen3ve).
 * Run from within the test directory.
 * Copyright (C)2014 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>

#include "nlutils.h"

// Tests nl_wait_get_return() and nl_popen3ve() without pipes.
int test_wait_return(void)
{
	char exe_path[1024];
	pid_t pid;
	int ret;

	INFO_OUT("Testing trivial nl_popen3ve() and nl_wait_get_return().\n");

	// Since all pipes are NULL, this is equivalent to execve(), but it
	// still helps to test the code.
	pid = nl_popen3ve(NULL, NULL, NULL, "/bin/false", (char *[]){"/bin/false", NULL}, environ);
	if(pid <= 0) {
		ERROR_OUT("Error opening /bin/false\n");
		return -1;
	}

	ret = nl_wait_get_return(pid);
	if(ret == -1) {
		ERROR_OUT("Error waiting for /bin/false\n");
		return -1;
	} else if(ret < -1) {
		ERROR_OUT("/bin/false encountered signal %d\n", -ret - 100);
		return -1;
	} else if(ret == 0) {
		ERROR_OUT("/bin/false returned 0 unexpectedly\n");
		return -1;
	}

	pid = nl_popen3ve(NULL, NULL, NULL, "/bin/true", (char *[]){"/bin/true", NULL}, environ);
	if(pid <= 0) {
		ERROR_OUT("Error running /bin/true\n");
		return -1;
	}

	ret = nl_wait_get_return(pid);
	if(ret == -1) {
		ERROR_OUT("Error waiting for /bin/true\n");
		return -1;
	} else if(ret < -1) {
		ERROR_OUT("/bin/true encountered signal %d\n", -ret - 100);
		return -1;
	} else if(ret > 0) {
		ERROR_OUT("/bin/true returned nonzero unexpectedly\n");
		return -1;
	}

	pid = nl_popen3ve(NULL, NULL, NULL, "/bin/sleep", (char *[]){"/bin/sleep", "5", NULL}, environ);
	if(pid <= 0) {
		ERROR_OUT("Error opening /bin/sleep\n");
		return -1;
	}

	if(kill(pid, SIGKILL)) {
		ERRNO_OUT("Error killing /bin/sleep");
		return -1;
	}

	ret = nl_wait_get_return(pid);
	if(ret == -1) {
		ERROR_OUT("Error waiting for /bin/sleep\n");
		return -1;
	} else if(ret < -1 && -(ret + 100) != SIGKILL) {
		ERROR_OUT("/bin/sleep encountered unexpected signal %d\n", -(ret + 100));
		return -1;
	} else if(ret >= 0) {
		ERROR_OUT("/bin/sleep returned unexpectedly\n");
		return -1;
	}

	// Test failure caused by missing command
	// TODO: Move this test into a separate nl_popen3ve test, add tests for non-NULL pipefd arguments
	snprintf(exe_path, sizeof(exe_path), "/no/way/this/exists/%x/%x/%x/anywhere", rand(), rand(), rand());
	pid = nl_popen3ve(NULL, NULL, NULL, exe_path, (char *[]){exe_path, NULL}, environ);
	if(pid != -1) {
		ERROR_OUT("Expected %s to fail to execute\n", exe_path);
		return -1;
	}

	return 0;
}

int test_popen3(void)
{
	struct nl_raw_data *raw_data;
	char *data;
	size_t size;

	int writefd, readfd, errfd;
	pid_t pid;
	int ret;

	INFO_OUT("Testing nl_popen3* and stream reading functions\n");

	// Test simple output reading
	data = nl_popen_readall("echo Test", &size);
	if(data == NULL) {
		ERROR_OUT("Error reading from simple nl_popen_readall echo test\n");
		return -1;
	}
	if(strcmp(data, "Test\n")) {
		ERROR_OUT("Expected echo data to be \"Test\\n\", got \"%s\"\n", data);
		return -1;
	}
	if(size != 5) {
		ERROR_OUT("Expected echo size to be 5, got %zu\n", size);
		return -1;
	}
	free(data);

	// Test large data (valgrind note: appears to allocate 3GB due to realloc up to 10MB in 16KB chunks)
	data = nl_popen_readall("dd if=/dev/zero bs=1048576 count=10 2>/dev/null", &size);
	if(data == NULL) {
		ERROR_OUT("Error reading 10MB from nl_popen_readall()\n");
		return -1;
	}
	if(size != 10485760) {
		ERROR_OUT("Expected 10MB read size to be 10485760, got %zu\n", size);
		return -1;
	}
	free(data);

	// Test output as well as input
	raw_data = nl_popenve_readall("/bin/cat", (char *[]){"/bin/cat", NULL}, environ,
			&(struct nl_raw_data){.data = "Test123", .size = 7});
	if(raw_data == NULL) {
		ERROR_OUT("Error reading from cat via nl_popenve_readall()\n");
		return -1;
	}
	if(strcmp(raw_data->data, "Test123")) {
		ERROR_OUT("Data from cat does not match: got '%s', expected 'Test123'.\n", raw_data->data);
		return -1;
	}
	if(raw_data->size != 7) {
		ERROR_OUT("Data from cat does not match expected size %d (got %zu).\n", 7, raw_data->size);
		return -1;
	}
	nl_destroy_data(raw_data);

	// Test all three streams
	pid = nl_popen3("echo 'Error stream' >&2; cat", &writefd, &readfd, &errfd);
	if(pid <= 0) {
		ERROR_OUT("Error opening echo+cat test.\n");
		return -1;
	}
	if(nl_write_stream(writefd, &(struct nl_raw_data){.data = "TestNL", .size = 6})) {
		ERROR_OUT("Error writing test data to echo+cat test.\n");
		return -1;
	}
	if(close(writefd)) {
		ERRNO_OUT("Error closing echo+cat test stdin fd");
		return -1;
	}

	if(CHECK_NULL(raw_data = nl_read_stream(readfd))) {
		return -1;
	}
	if(raw_data->size != 6) {
		ERROR_OUT("Data from stdout size is %zu, expected 6.\n", raw_data->size);
		return -1;
	}
	if(strcmp(raw_data->data, "TestNL")) {
		ERROR_OUT("Wrong data from stdout; expected 'TestNL', got '%s'\n", raw_data->data);
		return -1;
	}
	nl_destroy_data(raw_data);

	if(CHECK_NULL(raw_data = nl_read_stream(errfd))) {
		return -1;
	}
	if(raw_data->size != 13) {
		ERROR_OUT("Data from stdout size is %zu, expected 6.\n", raw_data->size);
		return -1;
	}
	if(strcmp(raw_data->data, "Error stream\n")) {
		ERROR_OUT("Wrong data from stdout; expected 'Error stream', got '%s'\n", raw_data->data);
		return -1;
	}
	nl_destroy_data(raw_data);

	if(close(readfd) || close(errfd)) {
		ERRNO_OUT("Error closing echo+cat test read+err file descriptors.\n");
		return -1;
	}

	ret = nl_wait_get_return(pid);
	if(ret) {
		ERROR_OUT("Expected echo+cat test to return 0, got %d\n", ret);
		return -1;
	}

	// Test environment passing
	raw_data = nl_popenve_readall("/bin/sh",
			(char *[]){ "/bin/sh", "-c", "echo \"${HOME}${TEST}\"", NULL },
			(char *[]){ "TEST=Nitrogen Logic", NULL },
			NULL);
	if(raw_data == NULL) {
		ERROR_OUT("Error running nl_popenve_readall environment override test\n");
		return -1;
	}
	if(strcmp(raw_data->data, "Nitrogen Logic\n")) {
		ERROR_OUT("Data mismatch on nl_popenve_readall environment test: got '%s', expected 'Nitrogen Logic'\n",
				raw_data->data);
		return -1;
	}
	nl_destroy_data(raw_data);

	return 0;
}

int main(void)
{
	INFO_OUT("Testing program execution functions.\n");

	if(test_wait_return() || test_popen3()) {
		ERROR_OUT("Program execution tests failed.\n");
		return -1;
	}

	INFO_OUT("Program execution tests succeeded.\n");

	return 0;
}
