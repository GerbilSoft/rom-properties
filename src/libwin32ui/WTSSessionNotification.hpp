/************************************************************************************
 * ROM Properties Page shell extension. (libwin32ui)                                *
 * WTSSessionNotification.hpp: WTSRegisterSessionNotification() RAII wrapper class. *
 *                                                                                  *
 * Copyright (c) 2016-2025 by David Korth.                                          *
 * SPDX-License-Identifier: GPL-2.0-or-later                                        *
 ************************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cassert>

// Windows SDK
#include "RpWin32_sdk.h"

namespace LibWin32UI {

/**
 * WTSRegisterSessionNotification() RAII wrapper.
 */
class WTSSessionNotification
{
public:
	inline WTSSessionNotification()
		: m_hWtsApi32_dll(LoadLibraryEx(_T("wtsapi32.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
	{}

	inline ~WTSSessionNotification() {
		if (m_hWtsApi32_dll) {
			FreeLibrary(m_hWtsApi32_dll);
		}
	}

public:
#ifndef NOTIFY_FOR_ALL_SESSIONS
#  define NOTIFY_FOR_ALL_SESSIONS 1
#endif
#ifndef NOTIFY_FOR_THIS_SESSION
#  define NOTIFY_FOR_THIS_SESSION 0
#endif

	/**
	 * Register for Remote Desktop session notifications.
	 * @param HWND hWnd
	 * @param dwFlags Flags
	 * @return TRUE on success; FALSE on error.
	 */
	inline BOOL registerSessionNotification(HWND hWnd, DWORD dwFlags) {
		if (!m_hWtsApi32_dll) {
			// DLL not found.
			SetLastError(ERROR_DLL_NOT_FOUND);
			return FALSE;
		}

		// Register for WTS session notifications.
		pfnWTSRegisterSessionNotification_t pfnWTSRegisterSessionNotification =
			(pfnWTSRegisterSessionNotification_t)GetProcAddress(m_hWtsApi32_dll, "WTSRegisterSessionNotification");
		if (pfnWTSRegisterSessionNotification) {
			return pfnWTSRegisterSessionNotification(hWnd, dwFlags);
		}

		// Not found in the DLL...
		// GetProcAddress() has already called SetLastError().
		return FALSE;
	}

	/**
	 * Unregister for Remote Desktop session notifications.
	 * @param HWND hWnd
	 * @return TRUE on success; FALSE on error.
	 */
	inline BOOL unRegisterSessionNotification(HWND hWnd) {
		if (!m_hWtsApi32_dll) {
			// DLL not found.
			SetLastError(ERROR_DLL_NOT_FOUND);
			return FALSE;
		}

		// Unregister for WTS session notifications.
		pfnWTSUnregisterSessionNotification_t pfnWTSUnRegisterSessionNotification =
			(pfnWTSUnregisterSessionNotification_t)GetProcAddress(m_hWtsApi32_dll, "WTSUnRegisterSessionNotification");
		if (pfnWTSUnRegisterSessionNotification) {
			return pfnWTSUnRegisterSessionNotification(hWnd);
		}

		// Not found in the DLL...
		// GetProcAddress() has already called SetLastError().
		return FALSE;
	}

	// Disable copy/assignment constructors.
public:
	WTSSessionNotification(const WTSSessionNotification &) = delete;
	WTSSessionNotification &operator=(const WTSSessionNotification &) = delete;

private:
	// TODO: Make it static and use reference counting?
	HMODULE m_hWtsApi32_dll;
	typedef BOOL (WINAPI *pfnWTSRegisterSessionNotification_t)(HWND hWnd, DWORD dwFlags);
	typedef BOOL (WINAPI *pfnWTSUnregisterSessionNotification_t)(HWND hWnd);
};

}
