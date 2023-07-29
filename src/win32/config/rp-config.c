/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * rp-config.c: Configuration stub.                                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * rp-config.c is a wrapper program for the Windows plugin.
 * It searches for the rom-properties.dll plugin and then
 * invokes a function to show the configuration dialog.
 */
#include "config.version.h"
#include "common.h"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"

// librpsecure
#include "librpsecure/os-secure.h"
#include "librpsecure/restrict-dll.h"

// C includes.
#include <locale.h>
#include <stdlib.h>
#include <string.h>

/**
 * rp_show_config_dialog() function pointer. (Win32 version)
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
#if defined(_M_ARM) || defined(__arm__) || \
    defined(_M_ARMT) || defined(__thumb__)
static const TCHAR rp_subdir[] = _T("arm\\");
#elif defined(_M_ARM64) || defined(__aarch64__)
static const TCHAR rp_subdir[] = _T("arm64\\");
#elif defined(_M_ARM64EC)
static const TCHAR rp_subdir[] = _T("arm64ec\\");
#elif defined(_M_IX86) || defined(__i386__)
static const TCHAR rp_subdir[] = _T("i386\\");
#elif defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__x86_64__)
static const TCHAR rp_subdir[] = _T("amd64\\");
#elif defined(_M_IA64) || defined(__ia64__)
static const TCHAR rp_subdir[] = _T("ia64\\");
#else
#  error Unsupported CPU architecture.
#endif

// NOTE: We're using strings because we have to use strings when
// looking up CLSIDs in the registry, so we might as well have the
// string format here instead of converting at runtime
// CLSID paths.
static const TCHAR CLSIDs[5][40] = {
	_T("{E51BC107-E491-4B29-A6A3-2A4309259802}"),	// RP_ExtractIcon
	_T("{84573BC0-9502-42F8-8066-CC527D0779E5}"),	// RP_ExtractImage
	_T("{2443C158-DF7C-4352-B435-BC9F885FFD52}"),	// RP_ShellPropSheetExt
	_T("{4723DF58-463E-4590-8F4A-8D9DD4F4355A}"),	// RP_ThumbnailProvider
	_T("{02C6AF01-3C99-497D-B3FC-E38CE526786B}"),	// RP_ShellIconOverlayIdentifier
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
	HMODULE hRpDll = LoadLibraryEx(dll_filename, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32); \
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
			ret = pfn(NULL, hInstance, lpCmdLine, nCmdShow); \
			FreeLibrary(hRpDll); \
			return ret; \
		} \
		FreeLibrary(hRpDll); \
	} \
} while (0)

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	HANDLE hSingleInstanceMutex;

	// Filename and path buffers.
	TCHAR *exe_path = NULL;	// MAX_PATH
	#define EXE_PATH_LEN (MAX_PATH)
	TCHAR *dll_filename = NULL;	// MAX_PATH+32
	#define DLL_FILENAME_LEN (MAX_PATH+32)

	TCHAR *last_backslash;
	HKEY hkeyCLSID = NULL;	// HKEY_CLASSES_ROOT\\CLSID
	DWORD exe_path_len;
	LONG lResult;
	unsigned int i;

	static const TCHAR prg_title[] = _T("ROM Properties Page Configuration");

	// Unused parameters (Win16 baggage)
	RP_UNUSED(hPrevInstance);

	/** librpsecure **/

	// Restrict DLL lookups.
	rp_secure_restrict_dll_lookups();

	// Set OS-specific security options.
	rp_secure_param_t param;
	param.bHighSec = FALSE;
	rp_secure_enable(param);

	/** main startup **/

	// Check if another instance of rp-config is already running.
	// References:
	// - https://stackoverflow.com/questions/4191465/how-to-run-only-one-instance-of-application
	// - https://stackoverflow.com/a/33531179
	// TODO: Localized window title?
	hSingleInstanceMutex = CreateMutex(NULL, TRUE, _T("Local\\com.gerbilsoft.rom-properties.rp-config"));
	if (!hSingleInstanceMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
		// Mutex already exists.
		// Set focus to the existing instance.
		// TODO: Localized window titles?
		HWND hWnd = FindWindow(_T("#32770"), prg_title);
		if (hWnd) {
			SetForegroundWindow(hWnd);
		}
		return EXIT_SUCCESS;
	}

	// Set the C locale.
	// TODO: C++ locale?
	setlocale(LC_ALL, "");
	// NOTE: Revert LC_CTYPE to "C" to fix UTF-8 output.
	setlocale(LC_CTYPE, "C");

#define FAIL_MESSAGE(msg) do { \
		MessageBox(NULL, (msg), prg_title, MB_ICONSTOP); \
		goto fail; \
	} while (0)

	// Get the executable path.
	// TODO: Support longer than MAX_PATH?
	exe_path = malloc(MAX_PATH*sizeof(TCHAR));
	if (!exe_path)
		FAIL_MESSAGE(_T("Failed to allocate memory for the EXE path."));
	SetLastError(ERROR_SUCCESS);	// required for XP
	exe_path_len = GetModuleFileName(hInstance, exe_path, EXE_PATH_LEN);
	if (exe_path_len == 0 || exe_path_len >= EXE_PATH_LEN || GetLastError() != ERROR_SUCCESS)
		FAIL_MESSAGE(_T("Failed to get the EXE path."));

	// Find the last backslash.
	last_backslash = _tcsrchr(exe_path, L'\\');
	if (last_backslash) {
		// NULL out everything after the backslash.
		last_backslash[1] = 0;
		exe_path_len = (DWORD)(last_backslash - exe_path + 1);
	} else {
		// Invalid path...
		// FIXME: Handle this.
		FAIL_MESSAGE(_T("EXE path is invalid."));
	}

	// Initialize dll_filename with exe_path.
	dll_filename = malloc(DLL_FILENAME_LEN*sizeof(TCHAR));
	if (!dll_filename)
		FAIL_MESSAGE(_T("Failed to allocate memory for the DLL filename."));
	memcpy(dll_filename, exe_path, exe_path_len*sizeof(TCHAR));

	// First, check for rom-properties.dll in rp-config.exe's directory.
	_tcscpy(&dll_filename[exe_path_len], _T("rom-properties.dll"));
	TRY_LOAD_DLL(dll_filename);

	// Check the architecture-specific subdirectory.
	_tcscpy(&dll_filename[exe_path_len], rp_subdir);
	// NOTE: -1 because _countof() includes the NULL terminator.
	_tcscpy(&dll_filename[exe_path_len + _countof(rp_subdir) - 1], _T("rom-properties.dll"));
	TRY_LOAD_DLL(dll_filename);

	// Check the CLSIDs.
	lResult = RegOpenKeyEx(HKEY_CLASSES_ROOT, _T("CLSID"), 0, KEY_ENUMERATE_SUB_KEYS, &hkeyCLSID);
	if (lResult != ERROR_SUCCESS)
		FAIL_MESSAGE(_T("Failed to open HKEY_CLASSES_ROOT\\CLSID."));

	// Need to open "HKCR\\CLSID\\{CLSID}\\InprocServer32".
	for (i = 0; i < _countof(CLSIDs); i++) {
		HKEY hkeyClass, hkeyInprocServer32;
		DWORD cbData, dwType;

		lResult = RegOpenKeyEx(hkeyCLSID, CLSIDs[i], 0, KEY_ENUMERATE_SUB_KEYS, &hkeyClass);
		if (lResult != ERROR_SUCCESS)
			continue;

		lResult = RegOpenKeyEx(hkeyClass, _T("InprocServer32"), 0, KEY_READ, &hkeyInprocServer32);
		if (lResult != ERROR_SUCCESS) {
			RegCloseKey(hkeyClass);
			continue;
		}

		// Read the default value to get the DLL filename.
		cbData = DLL_FILENAME_LEN*sizeof(TCHAR);
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
		if ((cbData % sizeof(TCHAR) != 0) || dll_filename[(cbData/sizeof(TCHAR))-1] != 0) {
			// Either this isn't a multiple of 2 bytes,
			// or there's no NULL terminator.
			continue;
		}

		if (dll_filename[0] != 0 && dwType == REG_EXPAND_SZ) {
			// Expand the string.
			// cchExpand includes the NULL terminator.
			TCHAR *wbuf;
			DWORD cchExpand = ExpandEnvironmentStrings(dll_filename, NULL, 0);
			if (cchExpand == 0) {
				// Error expanding the string.
				continue;
			}
			wbuf = malloc(cchExpand*sizeof(TCHAR));
			cchExpand = ExpandEnvironmentStrings(dll_filename, wbuf, cchExpand);
			if (cchExpand == 0) {
				// Error expanding the string.
				free(wbuf);
			} else {
				// String has been expanded.
				free(dll_filename);
				dll_filename = wbuf;
			}
		}

		// Attempt to load this DLL.
		TRY_LOAD_DLL(dll_filename);
	}

	// All options have failed...
	MessageBox(NULL, _T("Could not find rom-properties.dll.\n\n")
		_T("Please ensure the DLL is present in the same\ndirectory as rp-config.exe."),
		prg_title, MB_ICONWARNING);

fail:
	free(exe_path);
	free(dll_filename);
	if (hkeyCLSID) {
		RegCloseKey(hkeyCLSID);
	}
	return EXIT_FAILURE;
}
