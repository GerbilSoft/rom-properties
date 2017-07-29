/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * rp-config.c: Configuration stub.                                        *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

/**
 * rp-config.c is a wrapper program for the Windows plugin.
 * It searches for the rom-properties.dll plugin and then
 * invokes a function to show the configuration dialog.
 */
#include "config.version.h"

// LibRomData
// NOTE: We're not linking to LibRomData, so we can only
// use things that are defined in the headers.
#include "librpbase/common.h"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/secoptions.h"

// C includes.
#include <locale.h>
#include <stdlib.h>
#include <string.h>

/**
 * rp_show_config_dialog() function pointer.
 *
 * NOTE: This function pointer uses the same API as the function
 * expected by rundll32.exe.
 *
 * @param hWnd		[in] Parent window handle.
 * @param hInstance	[in] rp-config instance.
 * @param pszCmdLine	[in] Command line.
 * @param nCmdShow	[in] nCmdShow
 * @return 0 on success; non-zero on error.
 */
typedef int (CALLBACK *PFNRPSHOWCONFIGDIALOG)(HWND hWnd, HINSTANCE hInstance, LPSTR pszCmdLine, int nCmdShow);

// Architecture-specific subdirectory.
#if defined(__i386__) || defined(_M_IX86)
static const wchar_t rp_subdir[] = L"i386/";
#elif defined(__amd64__) || defined(_M_X64)
static const wchar_t rp_subdir[] = L"amd64/";
#else
#error Unsupported CPU architecture.
#endif

// NOTE: We're using strings because we have to use strings when
// looking up CLSIDs in the registry, so we might as well have the
// string format here instead of converting at runtime
// CLSID paths.
static const wchar_t CLSIDs[4][40] = {
	L"{E51BC107-E491-4B29-A6A3-2A4309259802}",	// RP_ExtractIcon
	L"{84573BC0-9502-42F8-8066-CC527D0779E5}",	// RP_ExtractImage
	L"{2443C158-DF7C-4352-B435-BC9F885FFD52}",	// RP_ShellPropSheetExt
	L"{4723DF58-463E-4590-8F4A-8D9DD4F4355A}",	// RP_ThumbnailProvider
};

/**
 * Try loading the ROM Properties Page DLL.
 *
 * If successful, rp_show_config_dialog() will be called and
 * its value will be returned to the caller.
 *
 * @param dll_filename DLL filename.
 */
#define TRY_LOAD_DLL(dll_filename) do { \
	HMODULE hRpDll = LoadLibrary(dll_filename); \
	if (hRpDll) { \
		/* Find the rp_show_config_dialog() function. */ \
		PFNRPSHOWCONFIGDIALOG pfn = (PFNRPSHOWCONFIGDIALOG)GetProcAddress(hRpDll, "rp_show_config_dialog"); \
		if (pfn) { \
			/* Run the function. */ \
			int ret; \
			free(exe_path); \
			free(dll_filename); \
			if (hkeyCLSID) { \
				RegCloseKey(hkeyCLSID); \
			} \
			ret = pfn(nullptr, hInstance, lpCmdLine, nCmdShow); \
			FreeLibrary(hRpDll); \
			return ret; \
		} \
		FreeLibrary(hRpDll); \
	} \
} while (0)

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	// Filename and path buffers.
	wchar_t *exe_path = NULL;	// MAX_PATH
	#define EXE_PATH_LEN (MAX_PATH)
	wchar_t *dll_filename = NULL;	// MAX_PATH+32
	#define DLL_FILENAME_LEN (MAX_PATH+32)

	wchar_t *last_backslash;
	HKEY hkeyCLSID = NULL;	// HKEY_CLASSES_ROOT\\CLSID
	DWORD exe_path_len;
	LONG lResult;
	unsigned int i;

	RP_UNUSED(hPrevInstance);

	// Set Win32 security options.
	secoptions_init();

	// Set the C locale.
	// TODO: C++ locale?
	setlocale(LC_ALL, "");

#define FAIL_MESSAGE(msg) do { \
		MessageBox(NULL, (msg), L"ROM Properties Page Configuration", MB_ICONSTOP); \
		goto fail; \
	} while (0)

	// Get the executable path.
	// TODO: Support longer than MAX_PATH?
	exe_path = malloc(MAX_PATH*sizeof(wchar_t));
	if (!exe_path)
		FAIL_MESSAGE(L"Failed to allocate memory for the EXE path.");
	exe_path_len = GetModuleFileName(hInstance, exe_path, EXE_PATH_LEN);
	if (exe_path_len == 0 || exe_path_len >= EXE_PATH_LEN)
		FAIL_MESSAGE(L"Failed to get the EXE path.");

	// Find the last backslash.
	last_backslash = wcsrchr(exe_path, L'\\');
	if (last_backslash) {
		// NULL out everything after the backslash.
		last_backslash[1] = 0;
		exe_path_len = (DWORD)(last_backslash - exe_path + 1);
	} else {
		// Invalid path...
		// FIXME: Handle this.
		FAIL_MESSAGE(L"EXE path is invalid.");
	}

	// Initialize dll_filename with exe_path.
	dll_filename = malloc(DLL_FILENAME_LEN*sizeof(wchar_t));
	if (!dll_filename)
		FAIL_MESSAGE(L"Failed to allocate memory for the DLL filename.");
	memcpy(dll_filename, exe_path, exe_path_len*sizeof(wchar_t));

	// First, check for rom-properties.dll in rp-config.exe's directory.
	wcscpy(&dll_filename[exe_path_len], L"rom-properties.dll");
	TRY_LOAD_DLL(dll_filename);

	// Check the architecture-specific subdirectory.
	wcscpy(&dll_filename[exe_path_len], rp_subdir);
	// NOTE: -1 because _countof() includes the NULL terminator.
	wcscpy(&dll_filename[exe_path_len + _countof(rp_subdir) - 1], L"rom-properties.dll");
	TRY_LOAD_DLL(dll_filename);

	// Check the CLSIDs.
	lResult = RegOpenKeyEx(HKEY_CLASSES_ROOT, L"CLSID", 0, KEY_ENUMERATE_SUB_KEYS, &hkeyCLSID);
	if (lResult != ERROR_SUCCESS)
		FAIL_MESSAGE(L"Failed to open HKEY_CLASSES_ROOT\\CLSID.");

	// Need to open "HKCR\\CLSID\\{CLSID}\\InprocServer32".
	for (i = 0; i < _countof(CLSIDs); i++) {
		HKEY hkeyClass, hkeyInprocServer32;
		DWORD cbData, dwType;

		lResult = RegOpenKeyEx(hkeyCLSID, CLSIDs[i], 0, KEY_ENUMERATE_SUB_KEYS, &hkeyClass);
		if (lResult != ERROR_SUCCESS)
			continue;

		lResult = RegOpenKeyEx(hkeyClass, L"InprocServer32", 0, KEY_READ, &hkeyInprocServer32);
		if (lResult != ERROR_SUCCESS) {
			RegCloseKey(hkeyClass);
			continue;
		}

		// Read the default value to get the DLL filename.
		cbData = DLL_FILENAME_LEN*sizeof(wchar_t);
		lResult = RegQueryValueEx(
			hkeyInprocServer32,	// hKey
			NULL,			// lpValueName
			NULL,			// lpReserved
			&dwType,		// lpType
			(LPBYTE)dll_filename,	// lpData
			&cbData);		// lpcbData
		RegCloseKey(hkeyInprocServer32);
		RegCloseKey(hkeyClass);

		if (lResult != ERROR_SUCCESS || (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
			continue;

		// Verify the NULL terminator.
		if ((cbData % sizeof(wchar_t) != 0) || dll_filename[(cbData/sizeof(wchar_t))-1] != 0) {
			// Either this isn't a multiple of 2 bytes,
			// or there's no NULL terminator.
			continue;
		}

		if (dll_filename[0] != 0 && dwType == REG_EXPAND_SZ) {
			// Expand the string.
			// cchExpand includes the NULL terminator.
			wchar_t *wbuf;
			DWORD cchExpand = ExpandEnvironmentStrings(dll_filename, nullptr, 0);
			if (cchExpand == 0) {
				// Error expanding the string.
				continue;
			}
			wbuf = malloc(cchExpand*sizeof(wchar_t));
			cchExpand = ExpandEnvironmentStrings(dll_filename, wbuf, cchExpand);
			if (cchExpand == 0) {
				// Error expanding the string.
				free(wbuf);
			}
			// String has been expanded.
			free(dll_filename);
			dll_filename = wbuf;
		}

		// Attempt to load this DLL.
		TRY_LOAD_DLL(dll_filename);
	}

	// All options have failed...
	MessageBox(NULL, L"Could not find rom-properties.dll.\n\n"
		L"Please ensure the DLL is present in the same\ndirectory as rp-config.exe.",
		L"ROM Properties Page Configuration", MB_ICONWARNING);

fail:
	free(exe_path);
	free(dll_filename);
	if (hkeyCLSID) {
		RegCloseKey(hkeyCLSID);
	}
	return EXIT_FAILURE;
}
