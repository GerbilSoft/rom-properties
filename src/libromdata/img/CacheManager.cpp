/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CacheManager.cpp: Local cache manager.                                  *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.libromdata.h"
#include "CacheManager.hpp"

// librpbase
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/RpFile.hpp"
#include "librpbase/file/FileSystem.hpp"
using namespace LibRpBase;
using namespace LibRpBase::FileSystem;

// libcachecommon
#include "libcachecommon/CacheKeys.hpp"

// OS-specific includes.
#ifdef _WIN32
# include "libwin32common/RpWin32_sdk.h"
# include "librpbase/TextFuncs_wchar.hpp"
#else /* !_WIN32 */
# include <sys/types.h>
# include <sys/wait.h>
# include <unistd.h>
# ifdef HAVE_POSIX_SPAWN
#  include <spawn.h>
# endif /* HAVE_POSIX_SPAWN */
#endif /* _WIN32 */

// C includes. (C++ namespace)
#include <ctime>

// C++ includes.
#include <string>
using std::string;
#ifdef _WIN32
using std::wstring;
#endif /* _WIN32 */

namespace LibRomData {

// Semaphore used to limit the number of simultaneous downloads.
// TODO: Determine the best number of simultaneous downloads.
// TODO: Test this on XP with IEIFLAG_ASYNC.
Semaphore CacheManager::m_dlsem(2);

/** Proxy server functions. **/
// NOTE: This is only useful for downloaders that
// can't retrieve the system proxy server normally.

/**
 * Get the proxy server.
 * @return Proxy server URL.
 */
string CacheManager::proxyUrl(void) const
{
	return m_proxyUrl;
}

/**
 * Set the proxy server.
 * @param proxyUrl Proxy server URL. (Use nullptr or blank string for default settings.)
 */
void CacheManager::setProxyUrl(const char *proxyUrl)
{
	if (proxyUrl) {
		m_proxyUrl = proxyUrl;
	} else {
		m_proxyUrl.clear();
	}
}

/**
 * Set the proxy server.
 * @param proxyUrl Proxy server URL. (Use blank string for default settings.)
 */
void CacheManager::setProxyUrl(const string &proxyUrl)
{
	m_proxyUrl = proxyUrl;
}

/**
 * Download a file.
 *
 * @param cache_key Cache key.
 *
 * The URL will be determined based on the cache key.
 *
 * If the file is present in the cache, the cached version
 * will be retrieved. Otherwise, the file will be downloaded.
 *
 * If the file was not found on the server, or it was not found
 * the last time it was requested, an empty string will be
 * returned, and a zero-byte file will be stored in the cache.
 *
 * @return Absolute path to the cached file.
 */
string CacheManager::download(const string &cache_key)
{
	// TODO: Only filter the cache key once.
	// Currently it's filtered twice:
	// - getCacheFilename() filters it
	// - We call filterCacheKey() before passing it to rp-download.

	// Check the main cache key.
	string cache_filename = LibCacheCommon::getCacheFilename(cache_key);
	if (cache_filename.empty()) {
		// Error obtaining the cache key filename.
		return string();
	}

	// Lock the semaphore to make sure we don't
	// download too many files at once.
	SemaphoreLocker locker(m_dlsem);

	// Check if the file already exists.
	if (!access(cache_filename, R_OK)) {
		// File exists.
		// Is it larger than 0 bytes?
		int64_t sz = filesize(cache_filename);
		if (sz == 0) {
			// File is 0 bytes, which indicates it didn't exist
			// on the server. If the file is older than a week,
			// try to redownload it.
			// TODO: Configurable time.
			// TODO: How should we handle errors?
			time_t filetime;
			if (get_mtime(cache_filename, &filetime) != 0)
				return string();

			const time_t systime = time(nullptr);
			if ((systime - filetime) < (86400*7)) {
				// Less than a week old.
				return string();
			}

			// More than a week old.
			// Delete the cache file and redownload it.
			if (delete_file(cache_filename) != 0)
				return string();
		} else if (sz > 0) {
			// File is larger than 0 bytes, which indicates
			// it was cached successfully.
			return cache_filename;
		}
	}

	// TODO: Add an option for "offline only".
	// Previously this was done by checking for a blank URL.
	// We don't have any offline-only databases right now, so
	// this has been temporarily removed.

	// Make sure the subdirectories exist.
	// NOTE: The filename portion MUST be kept in cache_filename,
	// since the last component is ignored by rmkdir().
	if (rmkdir(cache_filename) != 0) {
		// Error creating subdirectories.
		return string();
	}

	// Filter the cache key before passing it to rp-download.
	string filteredCacheKey = cache_key;
	int ret = LibCacheCommon::filterCacheKey(filteredCacheKey);
	assert(ret == 0);
	if (ret != 0) {
		// Shouldn't happen...
		return string();
	}

	// TODO: Split into OS-specific .cpp files and functions.
#if defined(_WIN32)
	// Execute rp-download.
	// The executable should be located in the DLL directory.
	extern TCHAR dll_filename[];
	tstring rp_download_exe = dll_filename;
	tstring::size_type bs = rp_download_exe.rfind(_T('\\'));
	if (bs == tstring::npos) {
		// Backslash not found.
		return string();
	}
	rp_download_exe.resize(bs+1);
	rp_download_exe += _T("rp-download.exe");

	// CreateProcessW() *can* modify the command line,
	// so we'll store in a tstring.
	// NOTE: Spaces are allowed in cache keys, so everything
	// needs to be quoted properly.
	tstring t_cache_key = U82T_s(cache_key);
	tstring t_cmd_line;
	t_cmd_line.reserve(rp_download_exe.size() + 5 + t_cache_key.size());
	t_cmd_line += _T('"');
	t_cmd_line += rp_download_exe;
	t_cmd_line += _T("\" \"");
	t_cmd_line += t_cache_key;
	t_cmd_line += _T('"');

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(si);

	BOOL bRet = CreateProcess(
		rp_download_exe.c_str(),	// lpApplicationName
		&t_cmd_line[0],			// lpCommandLine
		nullptr,			// lpProcessAttributes
		nullptr,			// lpThreadAttributes
		false,				// bInheritHandles
		CREATE_NO_WINDOW,		// dwCreationFlags
		nullptr,			// lpEnvironment
		nullptr,			// lpCurrentDirectory
		&si,				// lpStartupInfo
		&pi);				// lpProcessInformation
	if (!bRet) {
		// Error starting rp-download.exe.
		// TODO: Try the architecture-specific subdirectory?
		return string();
	}

	// Wait up to 10 seconds for the process to exit.
	DWORD dwRet = WaitForSingleObject(pi.hProcess, 10*1000);
	DWORD status = 0;
	GetExitCodeProcess(pi.hProcess, &status);
	if (dwRet != WAIT_OBJECT_0 || status == STILL_ACTIVE) {
		// Process either timed out or failed.
		TerminateProcess(pi.hProcess, EXIT_FAILURE);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return string();
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	if (status != 0) {
		// rp-download failed for some reason.
		return string();
	}
#elif defined(__APPLE__)
	// TODO: Mac stuff.
#elif defined(__linux__) || defined(__unix__)
	// Parameters.
#define rp_download_exe DIR_INSTALL_LIBEXEC "/rp-download"
	const char *const argv[3] = {
		rp_download_exe,
		cache_key.c_str(),
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
	pid_t pid;
	ret = posix_spawn(&pid, rp_download_exe,
		nullptr,	// file_actions
		nullptr,	// attrp
		(char *const *)argv, (char *const *)envp);
	if (ret != 0) {
		// Error creating the child process.
		return string();
	}
#else /* !HAVE_POSIX_SPAWN */
	// fork()/execve().
	pid_t pid = fork();
	if (pid == 0) {
		// Child process.
		int ret = execve(RP_DOWNLOAD_FILENAME, (char *const *)argv, (char *const *)envp);
		if (ret != 0) {
			// execve() failed.
			exit(EXIT_FAILURE);
		}
		assert(!"Shouldn't get here...");
		exit(EXIT_FAILURE);
	} else if (pid == -1) {
		// fork() failed.
		return string();
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
		kill(pid, SIGTERM);
		return string();
	}

	if (!ok) {
		// rp-download failed for some reason.
		return string();
	}
#endif

	// rp-download has successfully downloaded the file.
	return cache_filename;
}

/**
 * Check if a file has already been cached.
 * @param cache_key Cache key.
 * @return Filename in the cache, or empty string if not found.
 */
string CacheManager::findInCache(const string &cache_key)
{
	// Get the cache key filename.
	string cache_filename = LibCacheCommon::getCacheFilename(cache_key);
	if (cache_filename.empty()) {
		// Error obtaining the cache key filename.
		return string();
	}

	// Return the filename if the file exists.
	return (!access(cache_filename, R_OK) ? cache_filename : string());
}

}
