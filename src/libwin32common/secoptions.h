/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * secoptions.h: Security options for executables.                         *
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

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_SECOPTIONS_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_SECOPTIONS_H__

#include "RpWin32_sdk.h"

// C includes.
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

// DEP policy. (Vista SP1; later backported to XP SP3)
typedef BOOL (WINAPI *PFNSETPROCESSDEPPOLICY)(_In_ DWORD dwFlags);
#ifndef PROCESS_DEP_ENABLE
#define PROCESS_DEP_ENABLE 0x1
#endif
#ifndef PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION
#define PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION 0x2
#endif

// SetDllDirectory() (Win2003; later backported to XP SP1)
typedef BOOL (WINAPI *PFNSETDLLDIRECTORYW)(_In_opt_ LPCWSTR lpPathName);

// SetDefaultDllDirectories() (Win8; later backported to Vista and Win7)
typedef BOOL (WINAPI *PFNSETDEFAULTDLLDIRECTORIES)(_In_ DWORD DirectoryFlags);
#ifndef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
#define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR    0x00000100
#endif
#ifndef LOAD_LIBRARY_SEARCH_APPLICATION_DIR
#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#endif
#ifndef LOAD_LIBRARY_SEARCH_USER_DIRS
#define LOAD_LIBRARY_SEARCH_USER_DIRS       0x00000400
#endif
#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800
#endif
#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS    0x00001000
#endif

// HeapSetInformation() (WinXP)
typedef BOOL (WINAPI *PFNHEAPSETINFORMATION)
	(_In_opt_ HANDLE HeapHandle,
	 _In_ int HeapInformationClass,
	 _In_ PVOID HeapInformation,
	 _In_ SIZE_T HeapInformationLength);

#ifndef INLINE
#ifdef _MSC_VER
#define INLINE __inline
#else /* !_MSC_VER */
#define INLINE inline
#endif /* _MSC_VER */
#endif /* !INLINE */

/**
 * libromdata Windows executable initialization.
 * This sets various security options.
 * Reference: http://msdn.microsoft.com/en-us/library/bb430720.aspx
 *
 * @return 0 on success; non-zero on error.
 */
static INLINE int secoptions_init(void)
{
	BOOL bRet;
	HMODULE hKernel32;
	PFNSETPROCESSDEPPOLICY pfnSetProcessDEPPolicy;
	PFNSETDLLDIRECTORYW pfnSetDllDirectoryW;
	PFNSETDEFAULTDLLDIRECTORIES pfnSetDefaultDllDirectories;
	PFNHEAPSETINFORMATION pfnHeapSetInformation;

#ifndef NDEBUG
	// Make sure this function isn't called more than once.
	static unsigned char call_count = 0;
	assert(call_count == 0);
	call_count++;
#endif /* NDEBUG */

	// Using GetModuleHandleEx() to increase the refcount.
	bRet = GetModuleHandleEx(0, L"kernel32.dll", &hKernel32);
	if (!hKernel32) {
		// Should never happen...
		return GetLastError();
	}

	// Enable DEP/NX.
	// NOTE: DEP/NX should be specified in the PE header
	// using ld's --nxcompat, but we'll set it manually here,
	// just in case the linker doesn't support it.
	pfnSetProcessDEPPolicy = (PFNSETPROCESSDEPPOLICY)GetProcAddress(hKernel32, "SetProcessDEPPolicy");
	if (pfnSetProcessDEPPolicy) {
		pfnSetProcessDEPPolicy(PROCESS_DEP_ENABLE | PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION);
	}

	// Remove the current directory from the DLL search path.
	pfnSetDllDirectoryW = (PFNSETDLLDIRECTORYW)GetProcAddress(hKernel32, "SetDllDirectoryW");
	if (pfnSetDllDirectoryW) {
		pfnSetDllDirectoryW(L"");
	}

	// Only search the system directory for DLLs.
	// The Delay-Load helper will handle bundled DLLs at runtime.
	// NOTE: gdiplus.dll is not a "Known DLL", and since it isn't
	// delay-loaded, it may be loaded from the application directory...
	pfnSetDefaultDllDirectories = (PFNSETDEFAULTDLLDIRECTORIES)GetProcAddress(hKernel32, "SetDefaultDllDirectories");
	if (pfnSetDefaultDllDirectories) {
		pfnSetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
	}

	// Terminate the process if heap corruption is detected.
	// NOTE: Parameter 2 is usually type enum HEAP_INFORMATION_CLASS,
	// but this type isn't present in older versions of MinGW, so we're
	// using int instead.
	pfnHeapSetInformation = (PFNHEAPSETINFORMATION)GetProcAddress(hKernel32, "HeapSetInformation");
	if (pfnHeapSetInformation) {
		// HeapEnableTerminationOnCorruption == 1
		pfnHeapSetInformation(nullptr, 1, nullptr, 0);
	}

	FreeLibrary(hKernel32);
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_SECOPTIONS_H__ */
