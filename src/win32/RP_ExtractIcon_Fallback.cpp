/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon_Fallback.cpp: IExtractIcon implementation.               *
 * Fallback functions for unsupported files.                               *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RP_ExtractIcon.hpp"
#include "RP_ExtractIcon_p.hpp"

// librpbase, librpfile, libwin32common
#include "libwin32common/env_vars.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using LibWin32Common::RegKey;

// C++ STL classes.
using std::tstring;
using std::unique_ptr;

// COM smart pointer typedefs.
#ifndef _MSC_VER
// MSVC: Defined in comdefsp.h.
_COM_SMARTPTR_TYPEDEF(IClassFactory, IID_IClassFactory);
_COM_SMARTPTR_TYPEDEF(IPersistFile,  IID_IPersistFile);
#endif

/**
 * Use IExtractIconW from a fallback icon handler.
 * @param pExtractIconW Pointer to IExtractIconW interface.
 * @param phiconLarge Large icon.
 * @param phiconSmall Small icon.
 * @param nIconSize Icon sizes.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon_Private::DoExtractIconW(IExtractIconW *pExtractIconW,
	HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
	// Get the IPersistFile interface.
	IPersistFilePtr pPersistFile;
	HRESULT hr = pExtractIconW->QueryInterface(IID_PPV_ARGS(&pPersistFile));
	if (FAILED(hr)) {
		// Failed to get the IPersistFile interface.
		return ERROR_FILE_NOT_FOUND;
	}

	// Load the file.
	hr = pPersistFile->Load(U82W_s(this->filename), STGM_READ);
	if (FAILED(hr)) {
		// Failed to load the file.
		return ERROR_FILE_NOT_FOUND;
	}

	// Get the icon location.
	wchar_t szIconFileW[MAX_PATH];
	int nIconIndex;
	UINT wFlags;
	// TODO: Handle S_FALSE with GIL_DEFAULTICON?
	hr = pExtractIconW->GetIconLocation(0, szIconFileW, _countof(szIconFileW), &nIconIndex, &wFlags);
	if (FAILED(hr)) {
		// GetIconLocation() failed.
		return ERROR_FILE_NOT_FOUND;
	}

	if (wFlags & GIL_NOTFILENAME) {
		// Icon is not available on disk.
		// Use IExtractIcon::Extract().
		hr = pExtractIconW->Extract(szIconFileW, nIconIndex, phiconLarge, phiconSmall, nIconSize);
		return (SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_FILE_NOT_FOUND);
	} else {
		// Icon is available on disk.

		// PrivateExtractIcons() is published as of Windows XP SP1,
		// but it's "officially" private.
		// Reference: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-privateextracticonsw
		// TODO: Verify that hIcons[x] is NULL if only one size is found.
		// TODO: Verify which icon is extracted.
		// TODO: What if the size isn't found?

		// TODO: ANSI versions: Use GetProcAddress().
		HICON hIcons[2];
		UINT uRet = PrivateExtractIconsW(szIconFileW, nIconIndex,
				nIconSize, nIconSize, hIcons, nullptr, 2, 0);
		if (uRet == 0) {
			// No icons were extracted.
			return ERROR_FILE_NOT_FOUND;
		}

		// At least one icon was extracted.
		*phiconLarge = hIcons[0];
		*phiconSmall = hIcons[1];
	}
	return ERROR_SUCCESS;
}

/**
 * Use IExtractIconA from an old fallback icon handler.
 * @param pExtractIconA Pointer to IExtractIconW interface.
 * @param phiconLarge Large icon.
 * @param phiconSmall Small icon.
 * @param nIconSize Icon sizes.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon_Private::DoExtractIconA(IExtractIconA *pExtractIconA,
	HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
	// TODO: Verify that LPCOLESTR is still Unicode in IExtractIconA.
	// TODO: Needs testing.

	// Get the IPersistFile interface.
	IPersistFilePtr pPersistFile;
	HRESULT hr = pExtractIconA->QueryInterface(IID_PPV_ARGS(&pPersistFile));
	if (FAILED(hr)) {
		// Failed to get the IPersistFile interface.
		return ERROR_FILE_NOT_FOUND;
	}

	// Load the file.
	hr = pPersistFile->Load(U82W_s(this->filename), STGM_READ);
	if (FAILED(hr)) {
		// Failed to load the file.
		return ERROR_FILE_NOT_FOUND;
	}

	// Get the icon location.
	char szIconFileA[MAX_PATH];
	int nIconIndex;
	UINT wFlags;
	// TODO: Handle S_FALSE with GIL_DEFAULTICON?
	hr = pExtractIconA->GetIconLocation(0, szIconFileA, _countof(szIconFileA), &nIconIndex, &wFlags);
	if (FAILED(hr)) {
		// GetIconLocation() failed.
		return ERROR_FILE_NOT_FOUND;
	}

	if (wFlags & GIL_NOTFILENAME) {
		// Icon is not available on disk.
		// Use IExtractIcon::Extract().
		hr = pExtractIconA->Extract(szIconFileA, nIconIndex, phiconLarge, phiconSmall, nIconSize);
		return (SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_FILE_NOT_FOUND);
	} else {
		// Icon is available on disk.

		// PrivateExtractIcons() is published as of Windows XP SP1,
		// but it's "officially" private.
		// Reference: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-privateextracticonsa
		// TODO: Verify that hIcons[x] is NULL if only one size is found.
		// TODO: Verify which icon is extracted.
		// TODO: What if the size isn't found?

		// TODO: ANSI versions: Use GetProcAddress().
		HICON hIcons[2];
		UINT uRet = PrivateExtractIconsA(szIconFileA, nIconIndex,
				nIconSize, nIconSize, hIcons, nullptr, 2, 0);
		if (uRet == 0) {
			// No icons were extracted.
			return ERROR_FILE_NOT_FOUND;
		}

		// At least one icon was extracted.
		*phiconLarge = hIcons[0];
		*phiconSmall = hIcons[1];
	}
	return ERROR_SUCCESS;
}

/**
 * Get the icon index from an icon resource specification,
 * e.g. "C:\\Windows\\Some.DLL,1" .
 * @param szIconSpec Icon resource specification
 * @return Icon index, or 0 (default) if unknown.
 */
int RP_ExtractIcon_Private::getIconIndexFromSpec(LPCTSTR szIconSpec)
{
	// DefaultIcon format: "C:\\Windows\\Some.DLL,1"
	// TODO: Can the filename be quoted?
	// TODO: Better error codes?
	int nIconIndex;
	const TCHAR *const comma = _tcsrchr(szIconSpec, _T(','));
	if (!comma) {
		// No comma. Assume the default icon index.
		return 0;
	}

	// Found the comma.
	if (comma > szIconSpec && comma[1] != _T('\0')) {
		TCHAR *endptr = nullptr;
		errno = 0;
		nIconIndex = (int)_tcstol(&comma[1], &endptr, 10);
		if (errno == ERANGE || *endptr != 0) {
			// _tcstol() failed.
			// DefaultIcon is invalid, but we'll assume the index is 0.
			nIconIndex = 0;
		}
	} else {
		// Comma is the last character.
		// We'll assume the index is 0.
		nIconIndex = 0;
	}

	return nIconIndex;
}

/**
 * Fallback icon handler function. (internal)
 * This function reads the RP_Fallback key for fallback data.
 * @param hkey_Assoc File association key to check.
 * @param phiconLarge Large icon.
 * @param phiconSmall Small icon.
 * @param nIconSize Icon sizes.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon_Private::Fallback_int(RegKey &hkey_Assoc,
	HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
	// Is RP_Fallback present?
	RegKey hkey_RP_Fallback(hkey_Assoc, _T("RP_Fallback"), KEY_READ, false);
	if (!hkey_RP_Fallback.isOpen()) {
		return hkey_RP_Fallback.lOpenRes();
	}

	// Get the DefaultIcon key.
	DWORD dwType;
	tstring defaultIcon = hkey_RP_Fallback.read_expand(_T("DefaultIcon"), &dwType);
	if (defaultIcon.empty()) {
		// No default icon.
		return ERROR_FILE_NOT_FOUND;
	} else if (defaultIcon == _T("%1")) {
		// Forward to the IconHandler.
		const tstring iconHandler = hkey_RP_Fallback.read(_T("IconHandler"));
		if (iconHandler.empty()) {
			// No IconHandler.
			return ERROR_FILE_NOT_FOUND;
		}

		// Parse the CLSID string.
		// TODO: Use IIDFromString() instead to skip ProgID handling?
		// Reference: https://devblogs.microsoft.com/oldnewthing/20151015-00/?p=91351
		CLSID clsidIconHandler;
		HRESULT hr = CLSIDFromString(iconHandler.c_str(), &clsidIconHandler);
		if (FAILED(hr)) {
			// Failed to convert the CLSID from string.
			return ERROR_FILE_NOT_FOUND;
		}

		// Get the class object.
		IClassFactoryPtr pCF;
		hr = CoGetClassObject(clsidIconHandler, CLSCTX_INPROC_SERVER, nullptr, IID_PPV_ARGS(&pCF));
		if (FAILED(hr) || !pCF) {
			// Failed to get the IClassFactory.
			return ERROR_FILE_NOT_FOUND;
		}

		// Try getting the IExtractIconW interface.
		IExtractIconW *pExtractIconW;
		hr = pCF->CreateInstance(nullptr, IID_PPV_ARGS(&pExtractIconW));
		if (SUCCEEDED(hr) && pExtractIconW) {
			// Extract the icon.
			LONG lResult = DoExtractIconW(pExtractIconW, phiconLarge, phiconSmall, nIconSize);
			pExtractIconW->Release();
			return lResult;
		} else {
			// Try getting the IExtractIconA interface.
			IExtractIconA *pExtractIconA;
			hr = pCF->CreateInstance(nullptr, IID_PPV_ARGS(&pExtractIconA));
			if (SUCCEEDED(hr) && pExtractIconA) {
				// Extract the icon.
				LONG lResult = DoExtractIconA(pExtractIconA, phiconLarge, phiconSmall, nIconSize);
				pExtractIconA->Release();
				return lResult;
			} else {
				// Failed to get an IExtractIcon interface from the fallback class.
				return ERROR_FILE_NOT_FOUND;
			}
		}

		// Should not get here...
		return ERROR_INVALID_FUNCTION;
	}

	// DefaultIcon is set but IconHandler isn't, which means
	// the file's icon is stored as an icon resource.
	// TODO: Return filename+index in the main IExtractIconW handler?
	int nIconIndex = getIconIndexFromSpec(defaultIcon.c_str());
	// Remove the trailing comma at the end of defaultIcon, if present.
	size_t comma = defaultIcon.find_last_of(_T(','));
	if (comma != tstring::npos && comma > 0) {
		defaultIcon.resize(comma);
	}

	// PrivateExtractIcons() is published as of Windows XP SP1,
	// but it's "officially" private.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-privateextracticonsw
	// TODO: Verify that hIcons[x] is NULL if only one size is found.
	// TODO: Verify which icon is extracted.
	// TODO: What if the size isn't found?
	HICON hIcons[2];
	UINT uRet = PrivateExtractIcons(defaultIcon.c_str(), nIconIndex,
		nIconSize, nIconSize, hIcons, nullptr, 2, 0);
	if (uRet == 0) {
		// No icons were extracted.
		return ERROR_FILE_NOT_FOUND;
	}

	// At least one icon was extracted.
	*phiconLarge = hIcons[0];
	*phiconSmall = hIcons[1];
	return ERROR_SUCCESS;
}

/**
 * Fallback icon handler function.
 * @param phiconLarge Large icon.
 * @param phiconSmall Small icon.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon_Private::Fallback(HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
	// TODO: Check HKCU first.

	// Get the file extension.
	if (filename.empty()) {
		return ERROR_FILE_NOT_FOUND;
	}
	const char8_t *const file_ext = FileSystem::file_ext(filename.c_str());
	if (!file_ext) {
		// Invalid or missing file extension.
		return ERROR_FILE_NOT_FOUND;
	}
	const tstring ts_file_ext = U82T_c(file_ext);

	// Open the filetype key in HKCR.
	RegKey hkcr_Assoc(HKEY_CLASSES_ROOT, ts_file_ext.c_str(), KEY_READ, false);
	if (!hkcr_Assoc.isOpen()) {
		return hkcr_Assoc.lOpenRes();
	}

	// If we have a ProgID, check it first.
	tstring progID = hkcr_Assoc.read(nullptr);
	if (!progID.empty()) {
		// Custom ProgID is registered.
		// TODO: Get the correct top-level registry key.
		RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, progID.c_str(), KEY_READ, false);
		if (hkcr_ProgID.isOpen()) {
			LONG lResult = Fallback_int(hkcr_ProgID, phiconLarge, phiconSmall, nIconSize);
			if (lResult == ERROR_SUCCESS) {
				// ProgID icon extracted.
				return lResult;
			}
		}
	}

	// Check the filetype key.
	return Fallback_int(hkcr_Assoc, phiconLarge, phiconSmall, nIconSize);
}
