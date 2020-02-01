/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ThumbnailProvider.hpp: IThumbnailProvider implementation.            *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_ThumbnailProvider.hpp"
#include "RpImageWin32.hpp"

// librpbase, librptexture
using namespace LibRpBase;
using LibRpTexture::rp_image;

// RpFile_IStream
#include "RpFile_IStream.hpp"

// C++ STL classes.
using std::wstring;

// CLSID
const CLSID CLSID_RP_ThumbnailProvider =
	{0x4723df58, 0x463e, 0x4590, {0x8f, 0x4a, 0x8d, 0x9d, 0xd4, 0xf4, 0x35, 0x5a}};

/** RP_ThumbnailProvider_Private **/
#include "RP_ThumbnailProvider_p.hpp"

/** RP_ThumbnailProvider_Private **/

RP_ThumbnailProvider_Private::RP_ThumbnailProvider_Private()
	: file(nullptr)
	, pstream(nullptr)
	, grfMode(0)
{ }

RP_ThumbnailProvider_Private::~RP_ThumbnailProvider_Private()
{
	// pstream is owned by file,
	// so don't Release() it here.
	if (file) {
		file->unref();
	}
}

/** RP_ThumbnailProvider **/

RP_ThumbnailProvider::RP_ThumbnailProvider()
	: d_ptr(new RP_ThumbnailProvider_Private())
{ }

RP_ThumbnailProvider::~RP_ThumbnailProvider()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ThumbnailProvider::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(RP_ThumbnailProvider, IInitializeWithStream),
		QITABENT(RP_ThumbnailProvider, IThumbnailProvider),
		{ 0, 0 }
	};
#ifdef _MSC_VER
# pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::rp_QISearch(this, rgqit, riid, ppvObj);
}

/** IInitializeWithStream **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb761812(v=vs.85).aspx [Initialize()]

IFACEMETHODIMP RP_ThumbnailProvider::Initialize(IStream *pstream, DWORD grfMode)
{
	// Ignoring grfMode for now. (always read-only)
	RP_UNUSED(grfMode);

	// TODO: Check for network file systems and disable
	// thumbnailing if it's too slow.

	// Create an IRpFile wrapper for the IStream.
	RpFile_IStream *const file = new RpFile_IStream(pstream, true);
	if (!file->isOpen() || file->lastError() != 0) {
		// Error initializing the IRpFile.
		file->unref();
		return E_FAIL;
	}

	RP_D(RP_ThumbnailProvider);
	if (d->file) {
		// unref() the old file first.
		IRpFile *const old_file = d->file;
		d->file = file;
		old_file->unref();
	} else {
		// No old file to unref().
		d->file = file;
	}

	// Save the IStream and grfMode.
	d->pstream = pstream;
	d->grfMode = grfMode;
	return S_OK;
}

/** IThumbnailProvider **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb774612(v=vs.85).aspx [GetThumbnail()]

IFACEMETHODIMP RP_ThumbnailProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha)
{
	// Verify parameters:
	// - A stream must have been set by calling IInitializeWithStream::Initialize().
	// - phbmp and pdwAlpha must not be nullptr.
	RP_D(RP_ThumbnailProvider);
	if (!d->file || !phbmp || !pdwAlpha) {
		return E_INVALIDARG;
	}
	*phbmp = nullptr;
	*pdwAlpha = WTSAT_ARGB;

	int ret = d->thumbnailer.getThumbnail(d->file, cx, *phbmp);
	if (ret != 0 || !*phbmp) {
		// ROM is not supported. Use the fallback.
		return d->Fallback(cx, phbmp, pdwAlpha);
	}
	return S_OK;
}
