/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractImage_Register.cpp: IExtractImage implementation.             *
 * COM registration functions.                                             *
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
#include "RP_ExtractImage.hpp"
#include "RP_ExtractImage_p.hpp"

#include "RegKey.hpp"

// C++ includes.
#include <string>
using std::wstring;

#define IID_IExtractImage_String	TEXT("{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}")
#define CLSID_RP_ExtractImage_String	TEXT("{84573BC0-9502-42F8-8066-CC527D0779E5}")

/**
 * Register the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractImage::RegisterCLSID(void)
{
	static const wchar_t description[] = L"ROM Properties Page - Image Extractor";
	extern const wchar_t RP_ProgID[];

	// Register the COM object.
	LONG lResult = RegKey::RegisterComObject(__uuidof(RP_ExtractImage), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_ExtractImage), description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// COM object registered.
	return ERROR_SUCCESS;
}

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
	RegKey hkcr_IExtractImage(hkey_Assoc, L"ShellEx\\" IID_IExtractImage_String, KEY_READ|KEY_WRITE, true);
	if (!hkcr_IExtractImage.isOpen()) {
		return hkcr_IExtractImage.lOpenRes();
	}

	// Is a custom IExtractImage already registered?
	wstring clsid_reg = hkcr_IExtractImage.read(nullptr);
	if (!clsid_reg.empty() && clsid_reg != CLSID_RP_ExtractImage_String) {
		// Something else is registered.
		// Copy it to the fallback key.
		RegKey hkcr_RP_Fallback(hkey_Assoc, L"RP_Fallback", KEY_WRITE, true);
		if (!hkcr_RP_Fallback.isOpen()) {
			return hkcr_RP_Fallback.lOpenRes();
		}
		LONG lResult = hkcr_RP_Fallback.write(L"IExtractImage", clsid_reg);
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
 * @param hkcr HKEY_CLASSES_ROOT or user-specific classes root.
 * @param ext File extension, including the leading dot.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractImage::RegisterFileType(RegKey &hkcr, LPCWSTR ext)
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
	wstring progID = hkcr_ext.read(nullptr);
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
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractImage::UnregisterCLSID(void)
{
	extern const wchar_t RP_ProgID[];

	// Unegister the COM object.
	LONG lResult = RegKey::UnregisterComObject(__uuidof(RP_ExtractImage), RP_ProgID);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// TODO
	return ERROR_SUCCESS;
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

	// Open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkey_Assoc, L"ShellEx", KEY_READ, false);
	if (!hkcr_ShellEx.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		LONG lResult = hkcr_ShellEx.lOpenRes();
		if (lResult == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return lResult;
	}

	// Open the {IID_IExtractImage} key.
	RegKey hkcr_IExtractImage(hkcr_ShellEx, IID_IExtractImage_String, KEY_READ|KEY_WRITE, false);
	if (!hkcr_IExtractImage.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		LONG lResult = hkcr_IExtractImage.lOpenRes();
		if (lResult == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return lResult;
	}

	// Check if the default value matches the CLSID.
	wstring wstr_IExtractImage = hkcr_IExtractImage.read(nullptr);
	if (wstr_IExtractImage != CLSID_RP_ExtractImage_String) {
		// Not our IExtractImage.
		// We're done here.
		return ERROR_SUCCESS;
	}

	// Restore the fallbacks if we have any.
	wstring clsid_reg;
	RegKey hkcr_RP_Fallback(hkey_Assoc, L"RP_Fallback", KEY_READ|KEY_WRITE, false);
	if (hkcr_RP_Fallback.isOpen()) {
		// Read the fallback.
		clsid_reg = hkcr_RP_Fallback.read(L"IExtractImage");
	}

	if (!clsid_reg.empty()) {
		// Restore the IExtractImage.
		LONG lResult = hkcr_IExtractImage.write(nullptr, clsid_reg);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// No IExtractImage to restore.
		// Remove the current one.
		hkcr_IExtractImage.close();
		LONG lResult = hkcr_ShellEx.deleteSubKey(IID_IExtractImage_String);
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
		}
	}

	// Remove the fallbacks.
	LONG lResult = ERROR_SUCCESS;
	if (hkcr_RP_Fallback.isOpen()) {
		lResult = hkcr_RP_Fallback.deleteValue(L"IExtractImage");
		if (lResult == ERROR_FILE_NOT_FOUND) {
			lResult = ERROR_SUCCESS;
		}
	}

	// File type handler unregistered.
	return lResult;
}

/**
 * Unregister the file type handler.
 * @param hkcr HKEY_CLASSES_ROOT or user-specific classes root.
 * @param ext File extension, including the leading dot.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractImage::UnregisterFileType(RegKey &hkcr, LPCWSTR ext)
{
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
	wstring progID = hkcr_ext.read(nullptr);
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
