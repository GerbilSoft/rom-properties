/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon.hpp: IExtractIcon implementation.                        *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_RP_EXTRACTICON_H__
#define __ROMPROPERTIES_WIN32_RP_EXTRACTICON_H__

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

namespace LibWin32Common {
	class RegKey;
}

class RP_ExtractIcon_Private;

class UUID_ATTR("{E51BC107-E491-4B29-A6A3-2A4309259802}")
RP_ExtractIcon final : public LibWin32Common::ComBase3<IPersistFile, IExtractIconW, IExtractIconA>
{
	public:
		RP_ExtractIcon();
	protected:
		virtual ~RP_ExtractIcon();

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
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) final;

		// IPersist (IPersistFile base class)
		IFACEMETHODIMP GetClassID(CLSID *pClassID) final;
		// IPersistFile
		IFACEMETHODIMP IsDirty(void) final;
		IFACEMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode) final;
		IFACEMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember) final;
		IFACEMETHODIMP SaveCompleted(LPCOLESTR pszFileName) final;
		IFACEMETHODIMP GetCurFile(LPOLESTR *ppszFileName) final;

		// IExtractIconW
		IFACEMETHODIMP GetIconLocation(UINT uFlags, LPWSTR pszIconFile,
			UINT cchMax, int *piIndex, UINT *pwFlags) final;
		IFACEMETHODIMP Extract(LPCWSTR pszFile, UINT nIconIndex,
			HICON *phiconLarge, HICON *phiconSmall,
			UINT nIconSize) final;

		// IExtractIconA
		IFACEMETHODIMP GetIconLocation(UINT uFlags, LPSTR pszIconFile,
			UINT cchMax, int *piIndex, UINT *pwFlags) final;
		IFACEMETHODIMP Extract(LPCSTR pszFile, UINT nIconIndex,
			HICON *phiconLarge, HICON *phiconSmall,
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

#endif /* __ROMPROPERTIES_WIN32_RP_EXTRACTICON_H__ */
