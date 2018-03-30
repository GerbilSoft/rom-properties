/***************************************************************************
 * ROM Properties Page shell extension. (libi18n)                          *
 * i18n.c: Internationalization support code.                              *
 *                                                                         *
 * Copyright (c) 2017-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "i18n.h"
#ifdef _WIN32
# include "libwin32common/RpWin32_sdk.h"
#endif

#ifdef _WIN32
/**
 * Initialize the internationalization subsystem.
 * (Windows version)
 *
 * @return 0 on success; non-zero on error.
 */
int rp_i18n_init(void)
{
	// Windows: Use the application-specific locale directory.
	wchar_t dll_fullpath[MAX_PATH+16];
	wchar_t *bs;
	unsigned int path_len;
	DWORD dwAttrs;
	const char *base;

	// Get the current module filename.
	// NOTE: Delay-load only supports ANSI module names.
	// We'll assume it's ASCII and do a simple conversion to Unicode.
	SetLastError(ERROR_SUCCESS);
	DWORD dwResult = GetModuleFileName(HINST_THISCOMPONENT,
		dll_fullpath, _countof(dll_fullpath));
	if (dwResult == 0 || GetLastError() != ERROR_SUCCESS) {
		// Cannot get the current module filename.
		// TODO: Windows XP doesn't SetLastError() if the
		// filename is too big for the buffer.
		return -1;
	}

	// Find the last backslash in dll_fullpath[].
	bs = wcsrchr(dll_fullpath, L'\\');
	if (!bs) {
		// No backslashes...
		return -1;
	}

	// Append the "locale" subdirectory.
	wcscpy(bs+1, L"locale");
	dwAttrs = GetFileAttributes(dll_fullpath);
	if (dwAttrs == INVALID_FILE_ATTRIBUTES ||
	    !(dwAttrs & FILE_ATTRIBUTE_DIRECTORY))
	{
		// Not found, or not a directory.
		// Try one level up.
		// TODO: Only if the current subdirectory is
		// amd64/ or i386/.
		*bs = 0;
		bs = wcsrchr(dll_fullpath, L'\\');
		if (!bs) {
			// No backslashes...
			return -1;
		}

		// Append the "locale" subdirectory.
		wcscpy(bs+1, L"locale");
		dwAttrs = GetFileAttributes(dll_fullpath);
		if (dwAttrs == INVALID_FILE_ATTRIBUTES ||
		    !(dwAttrs & FILE_ATTRIBUTE_DIRECTORY))
		{
			// Not found, or not a subdirectory.
			return -1;
		}
	}

	// Found the locale subdirectory.
	// Bind the gettext domain.
	base = bindtextdomain(RP_I18N_DOMAIN, dirname);
	if (!base) {
		// bindtextdomain() failed.
		return -1;
	}

	// Initialized.
	return 0;
}

#else /* !_WIN32 */

/**
 * Initialize the internationalization subsystem.
 * (Unix/Linux version)
 *
 * @return 0 on success; non-zero on error.
 */
int rp_i18n_init(void)
{
	// Unix/Linux: Use the system-wide locale directory.
	const char *const base = bindtextdomain(RP_I18N_DOMAIN, DIR_INSTALL_LOCALE);
	if (!base) {
		// bindtextdomain() failed.
		return -1;
	}
	return 0;
}
#endif /* _WIN32 */
