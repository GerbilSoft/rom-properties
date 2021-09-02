/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * env_vars.hpp: Environment variable functions.                           *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "env_vars.hpp"

namespace LibWin32Common {

/**
 * Find the specified file in the system PATH.
 * @param szAppName File (usually an application name)
 * @return Full path, or empty string if not found.
 */
std::tstring findInPath(LPCTSTR szAppName)
{
	std::tstring full_path;
	TCHAR path_buf[4096];
	TCHAR expand_buf[MAX_PATH];

	DWORD dwRet = GetEnvironmentVariable(_T("PATH"), path_buf, _countof(path_buf));
	if (dwRet >= _countof(path_buf)) {
		// FIXME: Handle this?
		return full_path;
	}

	// Check each PATH entry.
	TCHAR *saveptr;
	for (const TCHAR *token = _tcstok_s(path_buf, _T(";"), &saveptr);
	     token != nullptr; token = _tcstok_s(nullptr, _T(";"), &saveptr))
	{
		// PATH may contain variables.
		dwRet = ExpandEnvironmentStrings(token, expand_buf, _countof(expand_buf));
		if (dwRet >= _countof(expand_buf)) {
			// FIXME: Handle this?
			continue;
		}

		full_path = expand_buf;
		full_path += _T('\\');
		full_path += szAppName;

		if (GetFileAttributes(full_path.c_str()) != INVALID_FILE_ATTRIBUTES) {
			// Found a match!
			return full_path;
		}
	}

	// No match.
	full_path.clear();
	return full_path;
}

}
