/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractImage_Fallback.cpp: IExtractImage implementation.             *
 * Fallback functions for unsupported files.                               *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RP_ExtractImage.hpp"
#include "RP_ExtractImage_p.hpp"

// librpbase, librpfile, libwin32common
using namespace LibRpBase;
using namespace LibRpFile;
using LibWin32UI::RegKey;

// C++ STL classes.
using std::tstring;

// COM smart pointer typedefs.
#ifndef _MSC_VER
// MSVC: Defined in comdefsp.h.
_COM_SMARTPTR_TYPEDEF(IClassFactory, IID_IClassFactory);
_COM_SMARTPTR_TYPEDEF(IPersistFile,  IID_IPersistFile);
#endif
_COM_SMARTPTR_TYPEDEF(IExtractImage, IID_IExtractImage);

/**
 * Fallback image handler function. (internal)
 * @param hkey_Assoc File association key to check.
 * @param phBmpImage
 * @return HRESULT.
 */
HRESULT RP_ExtractImage_Private::Fallback_int(RegKey &hkey_Assoc, HBITMAP *phBmpImage)
{
	// Is RP_Fallback present?
	RegKey hkey_RP_Fallback(hkey_Assoc, _T("RP_Fallback"), KEY_READ, false);
	if (!hkey_RP_Fallback.isOpen()) {
		return hkey_RP_Fallback.lOpenRes();
	}

	// Get the IExtractImage key.
	const tstring clsid_reg = hkey_RP_Fallback.read(_T("IExtractImage"));
	if (clsid_reg.empty()) {
		// No CLSID.
		return E_FAIL;
	}

	// Parse the CLSID string.
	// TODO: Use IIDFromString() instead to skip ProgID handling?
	// Reference: https://devblogs.microsoft.com/oldnewthing/20151015-00/?p=91351
	CLSID clsidExtractImage;
	HRESULT hr = CLSIDFromString(clsid_reg.c_str(), &clsidExtractImage);
	if (FAILED(hr)) {
		// Failed to convert the CLSID from string.
		return hr;
	}

	// Get the class object.
	IClassFactoryPtr pCF;
	hr = CoGetClassObject(clsidExtractImage, CLSCTX_INPROC_SERVER, nullptr, IID_PPV_ARGS(&pCF));
	if (FAILED(hr) || !pCF) {
		// Failed to get the IClassFactory.
		return hr;
	}

	// Try getting the IPersistFile interface.
	IPersistFilePtr pPersistFile;
	hr = pCF->CreateInstance(nullptr, IID_PPV_ARGS(&pPersistFile));
	if (FAILED(hr) || !pPersistFile) {
		// Failed to get the IPersistFile.
		return hr;
	}

	// Load the file.
	hr = pPersistFile->Load(this->olefilename.c_str(), STGM_READ);
	if (FAILED(hr)) {
		// Failed to load the file.
		return hr;
	}

	// Try getting the IExtractImage interface.
	IExtractImagePtr pExtractImage;
	hr = pPersistFile->QueryInterface(IID_PPV_ARGS(&pExtractImage));
	if (FAILED(hr) || !pExtractImage) {
		// Failed to get the IExtractImage.
		return hr;
	}

	// Get the image location.
	wchar_t szPathBuffer[MAX_PATH];	// NOTE: Not actually used.
	DWORD dwPriority = IEIT_PRIORITY_NORMAL;
	DWORD dwFlags = this->dwFlags;
	hr = pExtractImage->GetLocation(szPathBuffer, _countof(szPathBuffer),
		&dwPriority, &rgSize, dwRecClrDepth, &dwFlags);
	if (FAILED(hr) && hr != E_PENDING) {
		// Failed to get the image location.
		return hr;
	}

	// Get the image.
	return pExtractImage->Extract(phBmpImage);
}

/**
 * Fallback image handler function.
 * @param phBmpImage
 * @return HRESULT.
 */
HRESULT RP_ExtractImage_Private::Fallback(HBITMAP *phBmpImage)
{
	// TODO: Check HKCU first.

	// Get the file extension.
	if (unlikely(olefilename.empty())) {
		return E_INVALIDARG;
	}
	const wchar_t *wfile_ext = FileSystem::file_ext(olefilename);
	if (!wfile_ext) {
		// Invalid or missing file extension.
		return ERROR_FILE_NOT_FOUND;
	}

	// Open the filetype key in HKCR.
	RegKey hkey_Assoc(HKEY_CLASSES_ROOT, wfile_ext, KEY_READ, false);
	if (!hkey_Assoc.isOpen()) {
		return hkey_Assoc.lOpenRes();
	}

	// If we have a ProgID, check it first.
	const tstring progID = hkey_Assoc.read(nullptr);
	if (!progID.empty()) {
		// Custom ProgID is registered.
		// TODO: Get the correct top-level registry key.
		RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, progID.c_str(), KEY_READ, false);
		if (hkcr_ProgID.isOpen()) {
			HRESULT hr = Fallback_int(hkcr_ProgID, phBmpImage);
			if (SUCCEEDED(hr)) {
				// ProgID image extracted.
				return hr;
			}
		}
	}

	// Extract the image from the filetype key.
	return Fallback_int(hkey_Assoc, phBmpImage);
}
