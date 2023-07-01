/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * UpdateChecker.hpp: Update checker object for AboutTab.                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "UpdateChecker.hpp"

// librpbase
#include "librpbase/config/AboutTabText.hpp"
using namespace LibRpBase;

// libromdata
#include "libromdata/img/CacheManager.hpp"
using LibRomData::CacheManager;

// MSVCRT-specific
#include <process.h>

// C++ STL classes
using std::string;

UpdateChecker::UpdateChecker()
	: m_hThread(nullptr)
	, m_hWnd(nullptr)
	, m_errorMessage(nullptr)
	, m_updateVersion(0)
{ }

UpdateChecker::~UpdateChecker()
{
	if (m_hThread) {
		// Wait for the current thread to finish.
		// TODO: Wait indefinitely?
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = nullptr;
	}
}

/**
 * Update check thread procedure.
 * @param lpParameter Thread parameter (UpdateChecker object)
 * @return Error code.
 */
unsigned int WINAPI UpdateChecker::ThreadProc(LPVOID lpParameter)
{
	UpdateChecker *const updChecker = static_cast<UpdateChecker*>(lpParameter);

	// Download sys/version.txt and compare it to our version.
	// NOTE: Ignoring the fourth decimal (development flag).
	const char *const updateVersionUrl =
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::UpdateVersionUrl);
	const char *const updateVersionCacheKey =
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::UpdateVersionCacheKey);

	assert(updateVersionUrl != nullptr);
	assert(updateVersionCacheKey != nullptr);
	if (!updateVersionUrl || !updateVersionCacheKey) {
		// TODO: Show an error message.
		return 1;
	}

	// Download the version file.
	CacheManager cache;
	string cache_filename = cache.download(updateVersionCacheKey);
	if (cache_filename.empty()) {
		// Unable to download the version file.
		updChecker->m_errorMessage = C_("UpdateChecker", "Failed to download version file.");
		SendMessage(updChecker->m_hWnd, WM_UPD_ERROR, 0, 0);
		return 2;
	}

	// Read the version file.
	FILE *f = fopen(cache_filename.c_str(), "r");
	if (!f) {
		// TODO: Error code?
		updChecker->m_errorMessage = C_("UpdateChecker", "Failed to open version file.");
		SendMessage(updChecker->m_hWnd, WM_UPD_ERROR, 0, 0);
		return 3;
	}

	// Read the first line, which should contain a 4-decimal version number.
	char buf[256];
	char *fgret = fgets(buf, sizeof(buf), f);
	fclose(f);
	if (fgret == buf && ISSPACE(buf[0])) {
		updChecker->m_errorMessage = C_("UpdateChecker", "Version file is invalid.");
		SendMessage(updChecker->m_hWnd, WM_UPD_ERROR, 0, 0);
		return 4;
	}

	// Split into 4 elements using strtok_r(), and
	// convert to a 64-bit version. (ignoring the development flag)
	uint64_t updateVersion = 0;
	char *saveptr;
	const char *token = strtok_r(buf, ".", &saveptr);
	for (unsigned int i = 0; i < 3; i++, updateVersion <<= 16U) {
		if (!token) {
			// Missing token...
			updChecker->m_errorMessage = C_("UpdateChecker", "Version file is invalid.");
			SendMessage(updChecker->m_hWnd, WM_UPD_ERROR, 0, 0);
			return 5;
		}

		char *endptr;
		long long x = strtoll(token, &endptr, 10);
		if (x < 0 || *endptr != '\0') {
			updChecker->m_errorMessage = C_("UpdateChecker", "Version file is invalid.");
			SendMessage(updChecker->m_hWnd, WM_UPD_ERROR, 0, 0);
			return 6;
		}
		updateVersion |= ((uint64_t)x & 0xFFFFU);

		// Next token
		token = strtok_r(nullptr, ".", &saveptr);
	}

	// Fourth token is ignored but must be present.
	if (!token) {
		updChecker->m_errorMessage = C_("UpdateChecker", "Version file is invalid.");
		SendMessage(updChecker->m_hWnd, WM_UPD_ERROR, 0, 0);
		return 7;
	}
	// Fifth token must not be present.
	token = strtok_r(nullptr, ".", &saveptr);
	if (token) {
		updChecker->m_errorMessage = C_("UpdateChecker", "Version file is invalid.");
		SendMessage(updChecker->m_hWnd, WM_UPD_ERROR, 0, 0);
		return 8;
	}

	// Update version retrieved.
	updChecker->m_updateVersion = updateVersion;
	SendMessage(updChecker->m_hWnd, WM_UPD_RETRIEVED, 0, 0);
	return 0;
}

/**
 * Check for updates.
 * This will start a new thread and return immediately.
 * @param hWnd HWND to receive notification messages
 * @return True if successful; false if failed or the thread is already running.
 */
bool UpdateChecker::run(HWND hWnd)
{
	if (m_hThread) {
		// Wait for the current thread to finish.
		// TODO: Wait indefinitely?
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = nullptr;
	}

	// Reset all the variables.
	m_hWnd = hWnd;
	m_errorMessage = nullptr;
	m_updateVersion = 0;

	// Create the thread.
	m_hThread = (HANDLE)_beginthreadex(nullptr, 0, ThreadProc, this, 0, nullptr);
	if (!m_hThread) {
		m_errorMessage = C_("UpdateChecker", "Error creating thread.");
		SendMessage(m_hWnd, WM_UPD_ERROR, 0, 0);
		return false;
	}
	return true;
}
