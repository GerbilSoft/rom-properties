/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellIconOverlayIdentifier.cpp: IShellIconOverlayIdentifier          *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// librpbase
#include "librpbase/config.librpbase.h"
#include "common.h"

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "libwin32common/ComBase.hpp"

// CLSID common macros
#include "clsid_common.hpp"

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_ShellIconOverlayIdentifier;
}

namespace LibWin32UI {
	class RegKey;
}

class RP_ShellIconOverlayIdentifier_Private;

class UUID_ATTR("{02C6AF01-3C99-497D-B3FC-E38CE526786B}")
RP_ShellIconOverlayIdentifier final : public LibWin32Common::ComBase<IShellIconOverlayIdentifier>
{
public:
	RP_ShellIconOverlayIdentifier();
protected:
	~RP_ShellIconOverlayIdentifier() final;

private:
	typedef LibWin32Common::ComBase<IShellIconOverlayIdentifier> super;
	RP_DISABLE_COPY(RP_ShellIconOverlayIdentifier)
private:
	friend class RP_ShellIconOverlayIdentifier_Private;
	RP_ShellIconOverlayIdentifier_Private *const d_ptr;

public:
	CLSID_DECL_NOINLINE(RP_ShellIconOverlayIdentifier)

public:
	// IUnknown
	IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj) final;

	// IShellIconOverlayIdentifier
	IFACEMETHODIMP IsMemberOf(_In_ PCWSTR pwszPath, DWORD dwAttrib) final;
	IFACEMETHODIMP GetOverlayInfo(_Out_writes_(cchMax) PWSTR pwszIconFile, int cchMax, _Out_ int *pIndex, _Out_ DWORD *pdwFlags) final;
	IFACEMETHODIMP GetPriority(_Out_ int *pPriority) final;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ShellIconOverlayIdentifier, __MSABI_LONG(0x02c6af01), 0x3c99, 0x497d, 0xb3,0xfc, 0xe3, 0x8c, 0xe5, 0x26, 0x78, 0x6b);

// FIXME: MSYS2/MinGW-w64 (gcc-9.2.0-2, MinGW-w64 7.0.0.5524.2346384e-1)
// doesn't declare the UUID for IShellIconOverlayIdentifier for __uuidof() emulation.
__CRT_UUID_DECL(IShellIconOverlayIdentifier, __MSABI_LONG(0x0c6c4200), 0xc589, 0x11d0, 0x99,0x9a, 0x00, 0xc0, 0x4f, 0xd6, 0x55, 0xe1)
#endif
