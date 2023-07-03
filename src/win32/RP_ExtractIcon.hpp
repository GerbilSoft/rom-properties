/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon.hpp: IExtractIcon implementation.                        *
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
	extern const CLSID CLSID_RP_ExtractIcon;
}

namespace LibWin32UI {
	class RegKey;
}

class RP_ExtractIcon_Private;

class UUID_ATTR("{E51BC107-E491-4B29-A6A3-2A4309259802}")
RP_ExtractIcon final : public LibWin32Common::ComBase3<IPersistFile, IExtractIconW, IExtractIconA>
{
	public:
		RP_ExtractIcon();
	protected:
		~RP_ExtractIcon() final;

	private:
		typedef LibWin32Common::ComBase3<IPersistFile, IExtractIconW, IExtractIconA> super;
		RP_DISABLE_COPY(RP_ExtractIcon)
	private:
		friend class RP_ExtractIcon_Private;
		RP_ExtractIcon_Private *const d_ptr;

	public:
		CLSID_DECL(RP_ExtractIcon)
		FILETYPE_HANDLER_DECL(RP_ExtractIcon)

	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj) final;

		// IPersist (IPersistFile base class)
		IFACEMETHODIMP GetClassID(_Out_ CLSID *pClassID) final;
		// IPersistFile
		IFACEMETHODIMP IsDirty(void) final;
		IFACEMETHODIMP Load(_In_ LPCOLESTR pszFileName, DWORD dwMode) final;
		IFACEMETHODIMP Save(_In_ LPCOLESTR pszFileName, BOOL fRemember) final;
		IFACEMETHODIMP SaveCompleted(_In_ LPCOLESTR pszFileName) final;
		IFACEMETHODIMP GetCurFile(_In_ LPOLESTR *ppszFileName) final;

		// IExtractIconW
		IFACEMETHODIMP GetIconLocation(UINT uFlags,
			_Out_writes_(cchMax) LPWSTR pszIconFile, UINT cchMax,
			_Out_ int *piIndex, _Out_ UINT *pwFlags) final;
		IFACEMETHODIMP Extract(_In_ LPCWSTR pszFile, UINT nIconIndex,
			_Outptr_opt_ HICON *phiconLarge, _Outptr_opt_ HICON *phiconSmall,
			UINT nIconSize) final;

		// IExtractIconA
		IFACEMETHODIMP GetIconLocation(UINT uFlags,
			_Out_writes_(cchMax) LPSTR pszIconFile, UINT cchMax,
			_Out_ int *piIndex, _Out_ UINT *pwFlags) final;
		IFACEMETHODIMP Extract(_In_ LPCSTR pszFile, UINT nIconIndex,
			_Outptr_opt_ HICON *phiconLarge, _Outptr_opt_ HICON *phiconSmall,
			UINT nIconSize) final;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ExtractIcon, __MSABI_LONG(0xe51bc107), 0xe491, 0x4b29, 0xa6,0xa3, 0x2a, 0x43, 0x09, 0x25, 0x98, 0x02)

// FIXME: MSYS2/MinGW-w64 (gcc-9.2.0-2, MinGW-w64 7.0.0.5524.2346384e-1)
// doesn't declare the UUID for either IExtractIconW or IExtractIconA for
// __uuidof() emulation.
__CRT_UUID_DECL(IExtractIconA, __MSABI_LONG(0x000214eb), 0, 0, 0xc0,0, 0, 0, 0, 0, 0, 0x46)
__CRT_UUID_DECL(IExtractIconW, __MSABI_LONG(0x000214fa), 0, 0, 0xc0,0, 0, 0, 0, 0, 0, 0x46)
#endif
