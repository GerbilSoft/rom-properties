/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * UpdateChecker.hpp: Update checker object for AboutTab.                  *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
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

// C++ STL classes
using std::string;

/**
 * Check for updates.
 * This will start a new thread and return immediately.
 * @param hWnd HWND to receive notification messages
 * @return True if successful; false if failed or the thread is already running.
 */
bool UpdateChecker::run(HWND hWnd)
{
	// TODO: Run in a separate thread.
	m_errorMessage = nullptr;
	m_updateVersion = 0;

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
		return false;
	}

	// Download the version file.
	CacheManager cache;
	string cache_filename = cache.download(updateVersionCacheKey);
	if (cache_filename.empty()) {
		// Unable to download the version file.
		m_errorMessage = C_("UpdateChecker", "Failed to download version file.");
		SendMessage(hWnd, WM_UPD_ERROR, 0, 0);
		return true;
	}

	// Read the version file.
	FILE *f = fopen(cache_filename.c_str(), "r");
	if (!f) {
		// TODO: Error code?
		m_errorMessage = C_("UpdateChecker", "Failed to open version file.");
		SendMessage(hWnd, WM_UPD_ERROR, 0, 0);
		return true;
	}

	// Read the first line, which should contain a 4-decimal version number.
	char buf[256];
	char *fgret = fgets(buf, sizeof(buf), f);
	if (fgret == buf && ISSPACE(buf[0])) {
		m_errorMessage = C_("UpdateChecker", "Version file is invalid.");
		SendMessage(hWnd, WM_UPD_ERROR, 0, 0);
		return true;
	}

	// Split into 4 elements using strtok_r(), and
	// convert to a 64-bit version. (ignoring the development flag)
	uint64_t updateVersion = 0;
	char *saveptr;
	const char *token = strtok_r(buf, ".", &saveptr);
	for (unsigned int i = 0; i < 3; i++, updateVersion <<= 16U) {
		if (!token) {
			// Missing token...
			m_errorMessage = C_("UpdateChecker", "Version file is invalid.");
			SendMessage(hWnd, WM_UPD_ERROR, 0, 0);
			return true;
		}

		char *endptr;
		long long x = strtoll(token, &endptr, 10);
		if (x < 0 || *endptr != '\0') {
			m_errorMessage = C_("UpdateChecker", "Version file is invalid.");
			SendMessage(hWnd, WM_UPD_ERROR, 0, 0);
			return true;
		}
		updateVersion |= ((uint64_t)x & 0xFFFFU);

		// Next token
		token = strtok_r(nullptr, ".", &saveptr);
	}

	// Fourth token is ignored but must be present.
	if (!token) {
		m_errorMessage = C_("UpdateChecker", "Version file is invalid.");
		SendMessage(hWnd, WM_UPD_ERROR, 0, 0);
		return true;
	}
	// Fifth token must not be present.
	token = strtok_r(nullptr, ".", &saveptr);
	if (token) {
		m_errorMessage = C_("UpdateChecker", "Version file is invalid.");
		SendMessage(hWnd, WM_UPD_ERROR, 0, 0);
		return true;
	}

	// Update version retrieved.
	m_updateVersion = updateVersion;
	SendMessage(hWnd, WM_UPD_RETRIEVED, 0, 0);
	return true;
}
