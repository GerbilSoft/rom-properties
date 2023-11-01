/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ThumbnailProvider.hpp: IThumbnailProvider implementation.            *
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

// IThumbnailProvider
#include "thumbcache-wrapper.hpp"

// CLSID common macros
#include "clsid_common.hpp"

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_ThumbnailProvider;
}

namespace LibWin32UI {
	class RegKey;
}

// C++ includes.
#include <string>

class RP_ThumbnailProvider_Private;

class UUID_ATTR("{4723DF58-463E-4590-8F4A-8D9DD4F4355A}")
RP_ThumbnailProvider final : public LibWin32Common::ComBase2<IInitializeWithStream, IThumbnailProvider>
{
public:
	RP_ThumbnailProvider();
protected:
	~RP_ThumbnailProvider() final;

private:
	typedef LibWin32Common::ComBase2<IInitializeWithStream, IThumbnailProvider> super;
	RP_DISABLE_COPY(RP_ThumbnailProvider)
private:
	friend class RP_ThumbnailProvider_Private;
	RP_ThumbnailProvider_Private *const d_ptr;

public:
	CLSID_DECL(RP_ThumbnailProvider)
	FILETYPE_HANDLER_DECL(RP_ThumbnailProvider)

public:
	// IUnknown
	IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj) final;

	// IInitializeWithStream
	IFACEMETHODIMP Initialize(_In_ IStream *pstream, DWORD grfMode) final;

	// IThumbnailProvider
	IFACEMETHODIMP GetThumbnail(UINT cx, _Outptr_ HBITMAP *phbmp, _Out_ WTS_ALPHATYPE *pdwAlpha) final;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ThumbnailProvider, __MSABI_LONG(0x4723df58), 0x463e, 0x4590, 0x8f,0x4a, 0x8d, 0x9d, 0xd4, 0xf4, 0x35, 0x5a)
#endif
