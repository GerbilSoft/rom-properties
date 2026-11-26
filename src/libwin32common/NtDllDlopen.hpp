/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * NtDllDlopen.hpp: NTDLL.DLL functions using dlopen().                    *
 *                                                                         *
 * Copyright (c) 2026 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "RpWin32_sdk.h"
#include "sdk/ntdll-not-in-sdk.h"

namespace LibWin32Common {

// NTDLL dlopen() handler
class NtDllDlopen {
public:
	NtDllDlopen()
	{
		clearFnPtrs();
	}

	/**
	 * Initialize NTDLL.DLL function pointers.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int init(void);

	bool isInit(void) const
	{
		return (hNtDll != nullptr);
	}

private:
	/**
	 * Clear function pointers.
	 */
	void clearFnPtrs(void);

private:
	HMODULE hNtDll;

public:
#define NTDLL_DEF_PTR(fn) \
	typedef __typeof__(fn) *pfn_##fn##_t; \
	pfn_##fn##_t pfn_##fn

	NTDLL_DEF_PTR(NtClose);
	NTDLL_DEF_PTR(NtDeleteFile);
	NTDLL_DEF_PTR(NtOpenFile);
	NTDLL_DEF_PTR(NtQueryDirectoryFile);
	NTDLL_DEF_PTR(NtQueryInformationFile);
	NTDLL_DEF_PTR(NtSetInformationFile);
};

} // namespace LibWin32Common
