/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractImage_Register.cpp: IExtractImage implementation.             *
 * COM registration functions.                                             *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RP_ExtractImage.hpp"
#include "RP_ExtractImage_p.hpp"

// libwin32common
using LibWin32UI::RegKey;

// C++ STL classes.
using std::tstring;
using std::unique_ptr;

#define IID_IExtractImage_String	TEXT("{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")
#define CLSID_RP_ExtractImage_String	TEXT("{84573BC0-9502-42F8-8066-CC527D0779E5}")

/**
 * Register the file type handler.
 *
 * Internal version; this only registers for a single Classes key.
 * Called by the public version multiple times if a ProgID is registered.
 *
 * @param hkey_Assoc File association key to register under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractImage_Private::RegisterFileType(RegKey &hkey_Assoc)
{
	// Register as the image handler for this file association.

	// Create/open the "ShellEx\\{IID_IExtractImage}" key.
	// NOTE: This will recursively create the keys if necessary.
	RegKey hkcr_IExtractImage(hkey_Assoc, _T("ShellEx\\") IID_IExtractImage_String, KEY_READ|KEY_WRITE, true);
	if (!hkcr_IExtractImage.isOpen()) {
		return hkcr_IExtractImage.lOpenRes();
	}

	// Is a custom IExtractImage already registered?
	const tstring clsid_reg = hkcr_IExtractImage.read(nullptr);
	if (!clsid_reg.empty() && clsid_reg != CLSID_RP_ExtractImage_String) {
		// Something else is registered.
		// Copy it to the fallback key.
		RegKey hkcr_RP_Fallback(hkey_Assoc, _T("RP_Fallback"), KEY_WRITE, true);
		if (!hkcr_RP_Fallback.isOpen()) {
			return hkcr_RP_Fallback.lOpenRes();
		}
		LONG lResult = hkcr_RP_Fallback.write(_T("IExtractImage"), clsid_reg);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	}

	// NOTE: We're not skipping this even if the CLSID
	// is correct, just in case some setting needs to
	// be refreshed.

	// Set the IExtractImage to this CLSID.
	LONG lResult = hkcr_IExtractImage.write(nullptr, CLSID_RP_ExtractImage_String);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// File type handler registered.
	return ERROR_SUCCESS;
}

/**
 * Register the file type handler.
 * @param hkcr	[in] HKEY_CLASSES_ROOT or user-specific classes root.
 * @param ext	[in] File extension, including the leading dot.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractImage::RegisterFileType(_In_ RegKey &hkcr, _In_ LPCTSTR ext)
{
	// Open the file extension key.
	RegKey hkcr_ext(hkcr, ext, KEY_READ|KEY_WRITE, true);
	if (!hkcr_ext.isOpen()) {
		return hkcr_ext.lOpenRes();
	}

	// Register the main association.
	LONG lResult = RP_ExtractImage_Private::RegisterFileType(hkcr_ext);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Is a custom ProgID registered?
	// If so, and it has a DefaultIcon registered,
	// we'll need to update the custom ProgID.
	const tstring progID = hkcr_ext.read(nullptr);
	if (!progID.empty()) {
		// Custom ProgID is registered.
		RegKey hkcr_ProgID(hkcr, progID.c_str(), KEY_READ|KEY_WRITE, false);
		if (!hkcr_ProgID.isOpen()) {
			lResult = hkcr_ProgID.lOpenRes();
			if (lResult == ERROR_FILE_NOT_FOUND) {
				// ProgID not found. This is okay.
				return ERROR_SUCCESS;
			}
			return lResult;
		}
		lResult = RP_ExtractImage_Private::RegisterFileType(hkcr_ProgID);
	}

	// File type handler registered.
	return lResult;
}

/**
 * Unregister the file type handler.
 *
 * Internal version; this only unregisters for a single Classes key.
 * Called by the public version multiple times if a ProgID is registered.
 *
 * @param hkey_Assoc File association key to unregister under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractImage_Private::UnregisterFileType(RegKey &hkey_Assoc)
{
	// Unregister as the image handler for this file association.
	// NOTE: Continuing even if some keys are missing in case there
	// are other leftover keys.

	// Open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkey_Assoc, _T("ShellEx"), KEY_READ, false);
	if (!hkcr_ShellEx.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		LONG lResult = hkcr_ShellEx.lOpenRes();
		if (lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
		}
	}

	// Open the {IID_IExtractImage} key.
	unique_ptr<RegKey> phkcr_IExtractImage;
	if (hkcr_ShellEx.isOpen()) {
		phkcr_IExtractImage.reset(new RegKey(hkcr_ShellEx, IID_IExtractImage_String, KEY_READ|KEY_WRITE, false));
		if (phkcr_IExtractImage->isOpen()) {
			// Check if the default value matches the CLSID.
			const tstring str_IExtractImage = phkcr_IExtractImage->read(nullptr);
			if (str_IExtractImage != CLSID_RP_ExtractImage_String) {
				// Not our IExtractImage.
				phkcr_IExtractImage.reset(nullptr);
			}
		} else {
			// ERROR_FILE_NOT_FOUND is acceptable here.
			LONG lResult = phkcr_IExtractImage->lOpenRes();
			if (lResult != ERROR_FILE_NOT_FOUND) {
				return lResult;
			}
			phkcr_IExtractImage.reset(nullptr);
		}
	}

	// Restore the fallbacks if we have any.
	tstring clsid_reg;
	RegKey hkcr_RP_Fallback(hkey_Assoc, _T("RP_Fallback"), KEY_READ|KEY_WRITE, false);
	if (hkcr_RP_Fallback.isOpen()) {
		// Read the fallback.
		clsid_reg = hkcr_RP_Fallback.read(_T("IExtractImage"));
	}

	if (phkcr_IExtractImage) {
		if (!clsid_reg.empty()) {
			// Restore the IExtractImage.
			LONG lResult = phkcr_IExtractImage->write(nullptr, clsid_reg);
			if (lResult != ERROR_SUCCESS) {
				return lResult;
			}
		} else {
			// No IExtractImage to restore.
			// Remove the current one.
			phkcr_IExtractImage.reset();
			LONG lResult = hkcr_ShellEx.deleteSubKey(IID_IExtractImage_String);
			if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
				return lResult;
			}

			// If the "ShellEx" key is empty, delete it.
			if (hkcr_ShellEx.isKeyEmpty()) {
				hkcr_ShellEx.close();
				hkey_Assoc.deleteSubKey(_T("ShellEx"));
			}
		}
	}

	// Remove the fallbacks.
	if (hkcr_RP_Fallback.isOpen()) {
		LONG lResult = hkcr_RP_Fallback.deleteValue(_T("IExtractImage"));
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
		}

		// If the key is empty, delete it.
		if (hkcr_RP_Fallback.isKeyEmpty()) {
			hkcr_RP_Fallback.close();
			hkey_Assoc.deleteSubKey(_T("RP_Fallback"));
		}
	}

	// File type handler unregistered.
	return ERROR_SUCCESS;
}

/**
 * Unregister the file type handler.
 * @param hkcr	[in] HKEY_CLASSES_ROOT or user-specific classes root.
 * @param ext	[in,opt] File extension, including the leading dot.
 *
 * NOTE: ext can be NULL, in which case, hkcr is assumed to be
 * the registered file association.
 *
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractImage::UnregisterFileType(_In_ RegKey &hkcr, _In_opt_ LPCTSTR ext)
{
	if (!ext) {
		// Unregister from hkcr directly.
		return RP_ExtractImage_Private::UnregisterFileType(hkcr);
	}

	// Open the file extension key.
	RegKey hkcr_ext(hkcr, ext, KEY_READ|KEY_WRITE, false);
	if (!hkcr_ext.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		// In that case, it means we aren't registered.
		LONG lResult = hkcr_ext.lOpenRes();
		if (lResult == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return lResult;
	}

	// Unregister the main association.
	LONG lResult = RP_ExtractImage_Private::UnregisterFileType(hkcr_ext);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Is a custom ProgID registered?
	// If so, and it has a DefaultIcon registered,
	// we'll need to update the custom ProgID.
	const tstring progID = hkcr_ext.read(nullptr);
	if (!progID.empty()) {
		// Custom ProgID is registered.
		RegKey hkcr_ProgID(hkcr, progID.c_str(), KEY_READ|KEY_WRITE, false);
		if (!hkcr_ProgID.isOpen()) {
			lResult = hkcr_ProgID.lOpenRes();
			if (lResult == ERROR_FILE_NOT_FOUND) {
				// ProgID not found. This is okay.
				return ERROR_SUCCESS;
			}
			return lResult;
		}
		lResult = RP_ExtractImage_Private::UnregisterFileType(hkcr_ProgID);
	}

	// File type handler unregistered.
	return lResult;
}
