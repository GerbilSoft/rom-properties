/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ThumbnailProvider_Fallback.cpp: IThumbnailProvider implementation.   *
 * Fallback functions for unsupported files.                               *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "stdafx.h"
#include "RP_ThumbnailProvider.hpp"
#include "RP_ThumbnailProvider_p.hpp"

// libromdata
#include "libromdata/file/RpFile.hpp"
#include "libromdata/file/FileSystem.hpp"
using namespace LibRomData;

#include "RegKey.hpp"

// C++ includes.
#include <string>
using std::wstring;

// COM smart pointer typedefs.
#ifndef _MSC_VER
// MSVC: Defined in comdefsp.h.
_COM_SMARTPTR_TYPEDEF(IClassFactory, IID_IClassFactory);
#endif
_COM_SMARTPTR_TYPEDEF(IInitializeWithStream, IID_IInitializeWithStream);
_COM_SMARTPTR_TYPEDEF(IThumbnailProvider,    IID_IThumbnailProvider);

/**
 * Fallback thumbnail handler function. (internal)
 * @param hkey_Assoc File association key to check.
 * @param cx
 * @param phbmp
 * @param pdwAlpha
 * @return HRESULT.
 */
HRESULT RP_ThumbnailProvider_Private::Fallback_int(RegKey &hkey_Assoc,
	UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha)
{
	// Is RP_Fallback present?
	RegKey hkey_RP_Fallback(hkey_Assoc, L"RP_Fallback", KEY_READ, false);
	if (!hkey_RP_Fallback.isOpen()) {
		return hkey_RP_Fallback.lOpenRes();
	}

	// Get the IThumbnailProvider key.
	wstring clsid_reg = hkey_RP_Fallback.read(L"IThumbnailProvider");
	if (clsid_reg.empty()) {
		// No CLSID.
		return E_FAIL;
	}

	// Convert the CLSID from the string.
	CLSID clsidThumbnailProvider;
	HRESULT hr = CLSIDFromString(clsid_reg.c_str(), &clsidThumbnailProvider);
	if (FAILED(hr)) {
		// Failed to convert the CLSID from string.
		return hr;
	}

	// Get the class object.
	IClassFactoryPtr pCF;
	hr = CoGetClassObject(clsidThumbnailProvider, CLSCTX_INPROC_SERVER, nullptr, IID_PPV_ARGS(&pCF));
	if (FAILED(hr) || !pCF) {
		// Failed to get the IClassFactory.
		return hr;
	}

	// Try getting the IInitializeWithStream interface.
	// FIXME: WMP11 only has IInitializeWithItem.
	IInitializeWithStreamPtr pInitializeWithStream;
	hr = pCF->CreateInstance(nullptr, IID_PPV_ARGS(&pInitializeWithStream));
	if (FAILED(hr) || !pInitializeWithStream) {
		// Failed to get the IInitializeWithStream.
		return hr;
	}

	// Rewind the file.
	this->file->rewind();

	// Initialize the stream.
	hr = pInitializeWithStream->Initialize(this->pstream, this->grfMode);
	if (FAILED(hr)) {
		// Initialize() failed.
		return hr;
	}

	// Try getting the IThumbnailProvider interface.
	IThumbnailProviderPtr pThumbnailProvider;
	hr = pInitializeWithStream->QueryInterface(IID_PPV_ARGS(&pThumbnailProvider));
	if (FAILED(hr) || !pThumbnailProvider) {
		// Failed to get the IThumbnailProvider.
		return hr;
	}

	// Get the thumbnail.
	return pThumbnailProvider->GetThumbnail(cx, phbmp, pdwAlpha);
}

/**
 * Fallback thumbnail handler function.
 * @param cx
 * @param phbmp
 * @param pdwAlpha
 * @return HRESULT.
 */
HRESULT RP_ThumbnailProvider_Private::Fallback(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha)
{
	// TODO: Check HKCU first.

	// Get the file extension.
	const rp_string filename = this->file->filename();
	if (filename.empty()) {
		return E_INVALIDARG;
	}
	const rp_char *file_ext = FileSystem::file_ext(filename);
	if (!file_ext) {
		// Invalid or missing file extension.
		return E_INVALIDARG;
	}

	// Open the filetype key in HKCR.
	RegKey hkey_Assoc(HKEY_CLASSES_ROOT, RP2W_c(file_ext), KEY_READ, false);
	if (!hkey_Assoc.isOpen()) {
		return hkey_Assoc.lOpenRes();
	}

	// If we have a ProgID, check it first.
	wstring progID = hkey_Assoc.read(nullptr);
	if (!progID.empty()) {
		// Custom ProgID is registered.
		// TODO: Get the correct top-level registry key.
		RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, progID.c_str(), KEY_READ, false);
		if (hkcr_ProgID.isOpen()) {
			HRESULT hr = Fallback_int(hkcr_ProgID, cx, phbmp, pdwAlpha);
			if (SUCCEEDED(hr)) {
				// ProgID thumbnail extracted.
				return hr;
			}
		}
	}

	// Extract the thumbnail from the filetype key.
	return Fallback_int(hkey_Assoc, cx, phbmp, pdwAlpha);
}
