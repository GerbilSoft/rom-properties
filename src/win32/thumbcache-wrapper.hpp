/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * thumbcache-wrapper.hpp: thumbcache.h wrapper for MinGW-w64.             *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.win32.h"

#ifdef HAVE_THUMBCACHE_H
// System has thumbcache.h. Use it directly.
#  include <thumbcache.h>
#else /* !HAVE_THUMBCACHE_H */

// System does not have thumbcache.h.
// Declare the required interfaces here.
#include "common.h"

typedef enum WTS_ALPHATYPE {
	WTSAT_UNKNOWN	= 0,
	WTSAT_RGB	= 1,
	WTSAT_ARGB	= 2
} WTS_ALPHATYPE;

static const IID IID_IThumbnailProvider =
	{0xe357fccd, 0xa995, 0x4576, {0xb0, 0x1f, 0x23, 0x46, 0x30, 0x15, 0x4e, 0x96}};

MIDL_INTERFACE("e357fccd-a995-4576-b01f-234630154e96") NOVTABLE
IThumbnailProvider : public IUnknown
{
	public:
		virtual HRESULT STDMETHODCALLTYPE GetThumbnail(
			/* [in] */ UINT cx,
			/* [out] */ __RPC__deref_out_opt HBITMAP *phbmp,
			/* [out] */ __RPC__out WTS_ALPHATYPE *pdwAlpha) = 0;
};

// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(IThumbnailProvider, __MSABI_LONG(0xe357fccd), 0xa995, 0x4576, 0xb0,0x1f, 0x23, 0x46, 0x30, 0x15, 0x4e, 0x96)

#endif /* HAVE_THUMBCACHE_H */
