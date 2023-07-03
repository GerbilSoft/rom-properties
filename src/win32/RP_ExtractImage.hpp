/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractImage.hpp: IExtractImage implementation.                      *
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
	extern const CLSID CLSID_RP_ExtractImage;
}

namespace LibWin32UI {
	class RegKey;
}

class RP_ExtractImage_Private;

class UUID_ATTR("{84573BC0-9502-42F8-8066-CC527D0779E5}")
RP_ExtractImage final : public LibWin32Common::ComBase2<IPersistFile, IExtractImage2>
{
	public:
		RP_ExtractImage();
	protected:
		~RP_ExtractImage() final;

	private:
		typedef LibWin32Common::ComBase2<IPersistFile, IExtractImage> super;
		RP_DISABLE_COPY(RP_ExtractImage)
	private:
		friend class RP_ExtractImage_Private;
		RP_ExtractImage_Private *const d_ptr;

	public:
		CLSID_DECL(RP_ExtractImage)
		FILETYPE_HANDLER_DECL(RP_ExtractImage)

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
		IFACEMETHODIMP GetCurFile(_Outptr_ LPOLESTR *ppszFileName) final;

		// IExtractImage
		IFACEMETHODIMP GetLocation(
			_Out_writes_(cchMax) LPWSTR pszPathBuffer, DWORD cchMax,
			_Out_ DWORD *pdwPriority, _In_ const SIZE *prgSize,
			DWORD dwRecClrDepth, _Inout_ DWORD *pdwFlags) final;
		IFACEMETHODIMP Extract(_Outptr_ HBITMAP *phBmpImage) final;
		// IExtractImage2
		IFACEMETHODIMP GetDateStamp(_Out_ FILETIME *pDateStamp) final;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ExtractImage, __MSABI_LONG(0x84573bc0), 0x9502, 0x42f8, 0x80,0x66, 0xcc, 0x52, 0x7d, 0x07, 0x79, 0xe5)
#endif
