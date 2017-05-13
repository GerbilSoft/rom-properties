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
#include "libromdata/RpWin32_sdk.h"
#include "libromdata/Win32_ExeInit.hpp"

// C includes.
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

// rp_show_config_dialog export name.
// This function is decorated in order to prevent
// calling convention mismatches on i386.
// TODO: Check ARM?
#if defined(__i386__) || defined(_M_IX86)
static const char rp_show_config_dialog_export[] = "_rp_show_config_dialog@16";
static const wchar_t rp_subdir[] = L"i386/";
#elif defined(__amd64__) || defined(_M_X64)
static const char rp_show_config_dialog_export[] = "rp_show_config_dialog";
static const wchar_t rp_subdir[] = L"amd64/";
#else
#error Unsupported CPU architecture.
#endif

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
		PFNRPSHOWCONFIGDIALOG pfn = (PFNRPSHOWCONFIGDIALOG)GetProcAddress(hRpDll, rp_show_config_dialog_export); \
		if (pfn) { \
			/* Run the function. */ \
			free(exe_path); \
			free(dll_filename); \
			return pfn(nullptr, hInstance, lpCmdLine, nCmdShow); \
		} \
		FreeLibrary(hRpDll); \
	} \
} while (0)

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	// Filename and path buffers.
	wchar_t *exe_path = NULL;	// MAX_PATH
	wchar_t *dll_filename = NULL;	// MAX_PATH+32

	wchar_t *last_backslash;
	DWORD exe_path_len;

	((void)hPrevInstance);

	// libromdata Windows executable initialization.
	// This sets various security options.
	LibRomData_Win32_ExeInit();

	// Get the executable path.
	// TODO: Support longer than MAX_PATH?
	exe_path = malloc(MAX_PATH*sizeof(wchar_t));
	if (!exe_path)
		goto fail;
	exe_path_len = GetModuleFileName(hInstance, exe_path, MAX_PATH);
	if (exe_path_len == 0 || exe_path_len >= MAX_PATH) {
		// FIXME: Handle this.
		goto fail;
	}

	// Find the last backslash.
	last_backslash = wcsrchr(exe_path, L'\\');
	if (last_backslash) {
		// NULL out everything after the backslash.
		last_backslash[1] = 0;
		exe_path_len = (DWORD)(last_backslash - exe_path + 1);
	} else {
		// Invalid path...
		// FIXME: Handle this.
		goto fail;
	}

	// Initialize dll_filename with exe_path.
	dll_filename = malloc((MAX_PATH+32)*sizeof(wchar_t));
	if (!dll_filename)
		goto fail;
	memcpy(dll_filename, exe_path, exe_path_len*sizeof(wchar_t));

	// First, check for rom-properties.dll in rp-config.exe's directory.
	wcscpy(&dll_filename[exe_path_len], L"rom-properties.dll");
	TRY_LOAD_DLL(dll_filename);

	// Check the architecture-specific subdirectory.
	wcscpy(&dll_filename[exe_path_len], rp_subdir);
	// NOTE: -1 because _countof() includes the NULL terminator.
	wcscpy(&dll_filename[exe_path_len + _countof(rp_subdir) - 1], L"rom-properties.dll");
	TRY_LOAD_DLL(dll_filename);

	// FIXME: Search CLSIDs.
	// TODO: Show an error message.

fail:
	free(exe_path);
	free(dll_filename);
	return EXIT_FAILURE;
}
