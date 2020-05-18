/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AchLibNotify.cpp: Achievements class. (libnotify version)               *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"
#include "Achievements.hpp"

// OS-specific includes.
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef HAVE_POSIX_SPAWN
# include <spawn.h>
#endif /* HAVE_POSIX_SPAWN */

namespace LibRpBase {

/**
 * OS-specific achievement notify function.
 * @param id Achievement ID.
 * @return 0 on success; negative POSIX error code on error.
 */
int Achievements::unlock_os(ID id)
{
	// TODO: Full path to `notify-send`?
	static const char notify_send_exe[] = "notify-send";

	const char *desc = description(id);
	assert(desc != nullptr);
	if (!desc) {
		// No description...
		return -EINVAL;
	}

	// Parameters.
	const char *const argv[] = {
		notify_send_exe,
		"--app-name=rom-properties",
		"--category=x-gerbilsoft.rom-properties.achievement-unlocked",
		"--icon=answer-correct",
		"Achievement Unlocked",
		desc,
		nullptr
	};

	// TODO: Maybe we should close file handles...
	// NOTE: Using *p() versions because `notify-send` should be on the PATH.
#ifdef HAVE_POSIX_SPAWN
	// posix_spawnp()
	errno = 0;
	pid_t pid;
	int ret = posix_spawnp(&pid, notify_send_exe,
		nullptr,	// file_actions
		nullptr,	// attrp
		(char *const *)argv, environ);
	if (ret != 0) {
		// Error creating the child process.
		int err = errno;
		if (err == 0) {
			err = EIO;
		}
		return -err;
	}
#else /* !HAVE_POSIX_SPAWN */
	// fork()/execve().
	errno = 0;
	pid_t pid = fork();
	if (pid == 0) {
		// Child process.
		int ret = execvp(notify_send_exe, (char *const *)argv);
		if (ret != 0) {
			// execve() failed.
			exit(EXIT_FAILURE);
		}
		assert(!"Shouldn't get here...");
		exit(EXIT_FAILURE);
	} else if (pid == -1) {
		// fork() failed.
		int err = errno;
		if (err == 0) {
			err = EIO;
		}
		return -err;
	}
#endif /* HAVE_POSIX_SPAWN */

	// Parent process.
	// We want to wait on the PID at least once with WNOHANG;
	// otherwise, the child process will show up as <defunct>
	// when it exits.
	
	// Wait up to 1 second for the process to exit.
	// TODO: Report errors somewhere.
	bool ok = false;	// notify-send terminated successfully.
	bool waited = false;	// waitpid() was successful.
	int wstatus = 0;
	for (unsigned int i = 1*4; i > 0; i--) {
		pid_t wpid = waitpid(pid, &wstatus, WNOHANG);
		if (wpid == pid) {
			// Process has changed state.
			if (WIFEXITED(wstatus)) {
				// Child process has exited.
				// If the return status is non-zero, it failed.
				if (WEXITSTATUS(wstatus) != 0) {
					// Failure.
					break;
				}
				// Success!
				ok = true;
				waited = true;
				break;
			} else if (WIFSIGNALED(wstatus)) {
				// Child process terminated abnormally.
				waited = true;
				break;
			}
		}

		// Wait 250ms before checking again.
		usleep(250*1000);
	}

	if (!waited) {
		// Process did not complete.
		// TODO: Prevent race conditions by using waitid() instead of waitpid()?
		// TODO: Better error code?
		//kill(pid, SIGTERM);
		return -ECHILD;
	}

	if (!ok) {
		// notify-send failed for some reason.
		// TODO: Better error code?
		return -EIO;
	}

	// notify-send has successfully sent the notification.
	return 0;
}

}
