/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ThumbnailProvider.hpp: IThumbnailProvider implementation.            *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_ThumbnailProvider.hpp"
#include "RpImageWin32.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpTexture;

// RpFile_IStream
#include "file/RpFile_IStream.hpp"

// C++ STL classes
using std::shared_ptr;
using std::wstring;

// CLSID
const CLSID CLSID_RP_ThumbnailProvider =
	{0x4723df58, 0x463e, 0x4590, {0x8f, 0x4a, 0x8d, 0x9d, 0xd4, 0xf4, 0x35, 0x5a}};

/** RP_ThumbnailProvider_Private **/
#include "RP_ThumbnailProvider_p.hpp"

/** RP_ThumbnailProvider_Private **/

RP_ThumbnailProvider_Private::RP_ThumbnailProvider_Private()
	: pstream(nullptr)
	, grfMode(0)
{}

RP_ThumbnailProvider_Private::~RP_ThumbnailProvider_Private()
{
	if (pstream) {
		pstream->Release();
	}
}

/** RP_ThumbnailProvider **/

RP_ThumbnailProvider::RP_ThumbnailProvider()
	: d_ptr(new RP_ThumbnailProvider_Private())
{}

RP_ThumbnailProvider::~RP_ThumbnailProvider()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/implementing-iunknown-in-c-plus-plus

IFACEMETHODIMP RP_ThumbnailProvider::QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj)
{
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(RP_ThumbnailProvider, IInitializeWithStream),
		QITABENT(RP_ThumbnailProvider, IThumbnailProvider),
		{ 0, 0 }
	};
#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::rp_QISearch(this, rgqit, riid, ppvObj);
}

/** IInitializeWithStream **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/api/propsys/nf-propsys-iinitializewithstream-initialize [Initialize()]

IFACEMETHODIMP RP_ThumbnailProvider::Initialize(_In_ IStream *pstream, DWORD grfMode)
{
	// Ignoring grfMode for now. (always read-only)
	RP_UNUSED(grfMode);

	// TODO: Check for network file systems and disable
	// thumbnailing if it's too slow.

	// Create an IRpFile wrapper for the IStream.
	// NOTE: RpFile_IStream adds a reference to the IStream.
	IRpFilePtr file = std::make_shared<RpFile_IStream>(pstream, true);
	if (!file->isOpen() || file->lastError() != 0) {
		// Error initializing the IRpFile.
		return E_FAIL;
	}

	RP_D(RP_ThumbnailProvider);
	std::swap(d->file, file);

	// Save the IStream and grfMode.
	pstream->AddRef();
	d->pstream = pstream;
	d->grfMode = grfMode;
	return S_OK;
}

/** IThumbnailProvider **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/api/thumbcache/nf-thumbcache-ithumbnailprovider-getthumbnail [GetThumbnail()]

IFACEMETHODIMP RP_ThumbnailProvider::GetThumbnail(UINT cx, _Outptr_ HBITMAP *phbmp, _Out_ WTS_ALPHATYPE *pdwAlpha)
{
	// Verify parameters:
	// - A stream must have been set by calling IInitializeWithStream::Initialize().
	// - phbmp and pdwAlpha must not be nullptr.
	RP_D(RP_ThumbnailProvider);
	if (!d->file || !phbmp || !pdwAlpha) {
		return E_INVALIDARG;
	}
	*phbmp = nullptr;

	CreateThumbnail::GetThumbnailOutParams_t outParams;
	int ret = d->thumbnailer.getThumbnail(d->file, cx, &outParams);
	if (ret != 0) {
		// ROM is not supported. Use the fallback.
		return d->Fallback(cx, phbmp, pdwAlpha);
	}

	*phbmp = outParams.retImg;
	*pdwAlpha = (outParams.sBIT.alpha > 0 ? WTSAT_ARGB : WTSAT_RGB);
	return S_OK;
}
