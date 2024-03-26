/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ExecRpDownload_posix.cpp: Execute rp-download.exe. (POSIX)              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.libromdata.h"
#include "CacheManager.hpp"

// OS-specific includes.
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef HAVE_POSIX_SPAWN
# include <spawn.h>
#endif /* HAVE_POSIX_SPAWN */

// C++ includes.
#include <string>
using std::string;

namespace LibRomData {

/**
 * Execute rp-download. (POSIX version)
 * @param filteredCacheKey Filtered cache key.
 * @return 0 on success; negative POSIX error code on error.
 */
int CacheManager::execRpDownload(const string &filteredCacheKey)
{
	// TODO: Mac OS X path. (bundle?)
 	static constexpr char rp_download_exe[] = DIR_INSTALL_LIBEXEC "/rp-download";

	// Parameters.
	const char *const argv[3] = {
		rp_download_exe,
		filteredCacheKey.c_str(),
		nullptr
	};

	// Define a minimal environment for cURL.
	// This will include http_proxy and https_proxy if the proxy URL is set.
	// TODO: Separate proxies for http and https?
	// TODO: Only build this once?
	int pos[5] = {-1, -1, -1, -1, -1};
	int count = 0;
	std::string s_env;
	s_env.reserve(1024);

	// We want the HOME and USER variables.
	// If our proxy wasn't set, also get http_proxy and https_proxy
	// if they're set in the environment.
	const char *envtmp = getenv("HOME");
	if (envtmp && envtmp[0] != '\0') {
		pos[count++] = static_cast<int>(s_env.size());
		s_env += "HOME=";
		s_env += envtmp;
		s_env += '\0';
	}
	envtmp = getenv("USER");
	if (envtmp && envtmp[0] != '\0') {
		pos[count++] = static_cast<int>(s_env.size());
		s_env += "USER=";
		s_env += envtmp;
		s_env += '\0';
	}
	if (m_proxyUrl.empty()) {
		// Proxy URL is empty. Get the URLs from the environment.
		envtmp = getenv("http_proxy");
		if (envtmp && envtmp[0] != '\0') {
			pos[count++] = static_cast<int>(s_env.size());
			s_env += "http_proxy=";
			s_env += envtmp;
			s_env += '\0';
		}
		envtmp = getenv("https_proxy");
		if (envtmp && envtmp[0] != '\0') {
			pos[count++] = static_cast<int>(s_env.size());
			s_env += "https_proxy=";
			s_env += envtmp;
			s_env += '\0';
		}
	} else {
		// Proxy URL is set. Use it.
		pos[count++] = static_cast<int>(s_env.size());
		s_env += "http_proxy=" + m_proxyUrl;
		pos[count++] = static_cast<int>(s_env.size());
		s_env += "https_proxy=" + m_proxyUrl;
	}

	// Build envp.
	const char *envp[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
	unsigned int envp_idx = 0;
	for (unsigned int i = 0; i < 5; i++) {
		if (pos[i] >= 0) {
			envp[envp_idx++] = &s_env[pos[i]];
		}
	}

	// TODO: Maybe we should close file handles...
#ifdef HAVE_POSIX_SPAWN
	// posix_spawn()
	errno = 0;
	pid_t pid;
	int ret = posix_spawn(&pid, rp_download_exe,
		nullptr,	// file_actions
		nullptr,	// attrp
		(char *const *)argv, (char *const *)envp);
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
		int ret = execve(rp_download_exe, (char *const *)argv, (char *const *)envp);
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
	// Wait up to 10 seconds for the process to exit.
	// TODO: User-configurable timeout?
	// TODO: Report errors somewhere.
	bool ok = false;	// rp-download terminated successfully.
	bool waited = false;	// waitpid() was successful.
	int wstatus = 0;
	for (unsigned int i = 10*4; i > 0; i--) {
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
		kill(pid, SIGTERM);
		return -ECHILD;
	}

	if (!ok) {
		// rp-download failed for some reason.
		// TODO: Better error code?
		return -EIO;
	}

	// rp-download has successfully downloaded the file.
	return 0;
}

}
