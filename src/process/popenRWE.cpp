/**
 * Copyright 2009-2010 Bart Trojanowski <bart@jukie.net>
 * Licensed under GPLv2, or later, at your choosing.
 *
 * bidirectional popen() call
 *
 * @param rwepipe - int array of size three
 * @param exe - program to run
 * @param argv - argument list
 * @return pid or -1 on error
 *
 * The caller passes in an array of three integers (rwepipe), on successful
 * execution it can then write to element 0 (stdin of exe), and read from
 * element 1 (stdout) and 2 (stderr).
 */

#include "process/popenRWE.h"
#include <cerrno>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <unistd.h>

extern char **environ;

int popenRWE(int *rwepipe, const char *exe, const char *const argv[]) {
	int pipes[3][2] = {{-1,-1}, {-1,-1}, {-1,-1}};
	int error = 0;
	for (unsigned i = 0; i < 3 && !error; ++i) {
		if (pipe(pipes[i]) < 0) {
			error = errno;
			break;
		}
		for (auto &fd: pipes[i]) {
			if (fd < 3) {
				int copy = fcntl(fd, F_DUPFD, 3);
				if (copy < 0) { error = errno; break; }
				close(fd);
				fd = copy;
			}
			if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) { error = errno; break; }
		}
		int fd = pipes[i][i == 0? 1: 0];
		if (!error) {
			int flags = fcntl(fd, F_GETFL);
			if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) error = errno;
		}
	}
	pid_t pid = -1;
	if (!error) {
		posix_spawn_file_actions_t actions;
		error = posix_spawn_file_actions_init(&actions);
		if (!error) {
			for (unsigned i = 0; i < 3 && !error; ++i) {
				error = posix_spawn_file_actions_adddup2(&actions, pipes[i][i == 0? 0: 1], i);
			}
			posix_spawnattr_t attr;
			if (!error) {
				error = posix_spawnattr_init(&attr);
				if (!error) {
					sigset_t defaults, mask;
					sigemptyset(&defaults);
					sigaddset(&defaults, SIGPIPE);
					sigaddset(&defaults, SIGINT);
					sigaddset(&defaults, SIGTERM);
					sigemptyset(&mask);
					error = posix_spawnattr_setsigdefault(&attr, &defaults);
					if (!error) error = posix_spawnattr_setsigmask(&attr, &mask);
					if (!error) error = posix_spawnattr_setpgroup(&attr, 0);
					if (!error) error = posix_spawnattr_setflags(&attr,
						POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK);
					if (!error) error = posix_spawnp(&pid, exe, &actions, &attr,
						const_cast<char *const *>(argv), environ);
					posix_spawnattr_destroy(&attr);
				}
			}
			posix_spawn_file_actions_destroy(&actions);
		}
	}
	for (unsigned i = 0; i < 3; ++i) {
		for (unsigned j = 0; j < 2; ++j) {
			if (!error && j == (i == 0? 1u: 0u)) {
				rwepipe[i] = pipes[i][j];
			} else if (pipes[i][j] >= 0) {
				close(pipes[i][j]);
			}
		}
	}
	if (error) { errno = error; return -1; }
	return pid;
}
