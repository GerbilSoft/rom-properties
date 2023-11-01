/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * UpdateChecker.hpp: Update checker object for AboutTab.                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "libwin32common/RpWin32_sdk.h"

// C includes
#include <stdint.h>

// WM_UPD_ERROR: An error occurred fetching the update information.
// Call UpdateChecker::errorMessage() to get the error message.
// - wParam == unused
// - lParam == unused
#define WM_UPD_ERROR		(WM_USER + 0x1240)

// WM_UPD_RETRIEVED: The update version has been retrieved.
// Call UpdateChecker::updateVersion() to get the update version.
// - wParam == unused
// - lParam == unused
#define WM_UPD_RETRIEVED	(WM_USER + 0x1241)

class UpdateChecker
{
public:
	UpdateChecker();
	~UpdateChecker();

private:
	RP_DISABLE_COPY(UpdateChecker)

public:
	/**
		* Check for updates.
		* This will start a new thread and return immediately.
		* @param hWnd HWND to receive notification messages
		* @return True if successful; false if failed or the thread is already running.
		*/
	bool run(HWND hWnd);

private:
	/**
	* Update check thread procedure.
	* @param lpParameter Thread parameter (UpdateChecker object)
	* @return Error code.
	*/
	static unsigned int WINAPI ThreadProc(LPVOID lpParameter);

public:
	inline const char *errorMessage(void) const
	{
		return m_errorMessage;
	}

	inline uint64_t updateVersion(void) const
	{
		return m_updateVersion;
	}

protected:
	// Active thread
	HANDLE m_hThread;

	// hWnd to send messages to
	HWND m_hWnd;

	// Error message for WM_UPD_ERROR
	const char *m_errorMessage;

	// Update version
	uint64_t m_updateVersion;
};
