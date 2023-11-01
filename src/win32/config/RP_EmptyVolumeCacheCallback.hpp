/********************************************************************************
 * ROM Properties Page shell extension. (Win32)                                 *
 * RP_EmptyVolumeCacheCallback.cpp: RP_EmptyVolumeCacheCallback implementation. *
 *                                                                              *
 * Copyright (c) 2016-2023 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                                    *
 ********************************************************************************/

#pragma once

/**
 * IEmptyVolumeCacheCallback implementation.
 * Used for thumbnail cache cleaning on Windows Vista and later.
 *
 * NOTE: This class is NOT registered with the system.
 * Therefore, we aren't defining a CLSID.
 */

#include "common.h"
#include "libwin32common/ComBase.hpp"

class RP_EmptyVolumeCacheCallback final : public LibWin32Common::ComBase<IEmptyVolumeCacheCallBack>
{
public:
	explicit RP_EmptyVolumeCacheCallback(HWND hProgressBar);

private:
	typedef LibWin32Common::ComBase<RP_EmptyVolumeCacheCallback> super;
	RP_DISABLE_COPY(RP_EmptyVolumeCacheCallback)

public:
	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) final;

public:
	// IEmptyVolumeCacheCallBack
	IFACEMETHODIMP ScanProgress(DWORDLONG dwlSpaceUsed, DWORD dwFlags, LPCWSTR pcwszStatus) final;
	IFACEMETHODIMP PurgeProgress(DWORDLONG dwlSpaceFreed, DWORDLONG dwlSpaceToFree, DWORD dwFlags, LPCWSTR pcwszStatus) final;

private:
	HWND m_hProgressBar;		// Progress bar to update.
public:
	unsigned int m_baseProgress;	// Base progress value.
};

#ifdef __CRT_UUID_DECL
// FIXME: MSYS2/MinGW-w64 (gcc-9.2.0-2, MinGW-w64 7.0.0.5524.2346384e-1)
// doesn't declare the UUID for IEmptyVolumeCacheCallBack for __uuidof() emulation.
__CRT_UUID_DECL(IEmptyVolumeCacheCallBack, __MSABI_LONG(0x6e793361), 0x73c6, 0x11d0, 0x84,0x69, 0x00, 0xaa, 0x00, 0x44, 0x29, 0x01)
#endif
