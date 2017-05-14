/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * Win32_ExeInit.hpp: Win32 common executable initialization.              *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_WIN32_EXEINIT_H__
#define __ROMPROPERTIES_LIBROMDATA_WIN32_EXEINIT_H__

#ifndef _WIN32
#error Win32_ExeInit.hpp should only be included on Windows.
#endif /* !_WIN32 */

#include "RpWin32_sdk.h"

// DEP policy. (requires _WIN32_WINNT >= 0x0600)
#ifndef PROCESS_DEP_ENABLE
#define PROCESS_DEP_ENABLE 0x1
#endif
#ifndef PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION
#define PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION 0x2
#endif

#ifdef __cplusplus
namespace LibRomData {
#endif

/**
 * libromdata Windows executable initialization.
 * This sets various security options.
 * Reference: http://msdn.microsoft.com/en-us/library/bb430720.aspx
 *
 * @return 0 on success; non-zero on error.
 */
#ifdef __cplusplus
static int Win32_ExeInit(void)
#else
static int LibRomData_Win32_ExeInit(void)
#endif
{
	typedef BOOL (WINAPI *PFNSETDEP)(DWORD dwFlags);
	typedef BOOL (WINAPI *PFNSETDLLDIRW)(LPCWSTR lpPathName);
	typedef BOOL (WINAPI *PFNHEAPSETINFORMATION)
		(HANDLE HeapHandle, int HeapInformationClass,
		 PVOID HeapInformation, SIZE_T HeapInformationLength);

	HMODULE hKernel32;
	PFNSETDEP pfnSetDep;
	PFNSETDLLDIRW pfnSetDllDirectoryW;
	PFNHEAPSETINFORMATION pfnHeapSetInformation;

	hKernel32 = LoadLibrary(L"kernel32.dll");
	if (!hKernel32) {
		// Should never happen...
		return GetLastError();
	}

	// Enable DEP/NX. (WinXP SP3, Vista, and later.)
	// NOTE: DEP/NX should be specified in the PE header
	// using ld's --nxcompat, but we'll set it manually here,
	// just in case the linker doesn't support it.
	pfnSetDep = (PFNSETDEP)GetProcAddress(hKernel32, "SetProcessDEPPolicy");
	if (pfnSetDep) {
		pfnSetDep(PROCESS_DEP_ENABLE);
	}

	// Remove the current directory from the DLL search path.
	pfnSetDllDirectoryW = (PFNSETDLLDIRW)GetProcAddress(hKernel32, "SetDllDirectoryW");
	if (pfnSetDllDirectoryW) {
		pfnSetDllDirectoryW(L"");
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

#endif /* __ROMPROPERTIES_LIBROMDATA_WIN32_EXEINIT_H__ */
