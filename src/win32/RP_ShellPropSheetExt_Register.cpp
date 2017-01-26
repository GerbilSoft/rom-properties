/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt_Register.cpp: IShellPropSheetExt implementation.   *
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
#include "RP_ShellPropSheetExt.hpp"
#include "RegKey.hpp"

// C++ includes.
#include <string>
using std::wstring;

/**
 * Register the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ShellPropSheetExt::RegisterCLSID(void)
{
	static const wchar_t description[] = L"ROM Properties Page - Property Sheet";
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ShellPropSheetExt), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Register the COM object.
	lResult = RegKey::RegisterComObject(__uuidof(RP_ShellPropSheetExt), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_ShellPropSheetExt), description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// COM object registered.
	return ERROR_SUCCESS;
}

/**
 * Register the file type handler.
 * @param hkcr HKEY_CLASSES_ROOT or user-specific classes root.
 * @param ext File extension, including the leading dot.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ShellPropSheetExt::RegisterFileType(RegKey &hkcr, LPCWSTR ext)
{
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ShellPropSheetExt), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Open the file extension key.
	RegKey hkcr_ext(hkcr, ext, KEY_WRITE, true);
	if (!hkcr_ext.isOpen()) {
		return hkcr_ext.lOpenRes();
	}

	// Register as a property sheet handler for this file association.

	// Create/open the "ShellEx\\PropertySheetHandlers\\rom-properties" key.
	// NOTE: This will recursively create the keys if necessary.
	wstring keyname = L"ShellEx\\PropertySheetHandlers\\";
	keyname += RP_ProgID;
	RegKey hkcr_PropSheet(hkcr_ext, keyname.c_str(), KEY_WRITE, true);
	if (!hkcr_PropSheet.isOpen()) {
		return hkcr_PropSheet.lOpenRes();
	}
	// Set the default value to this CLSID.
	lResult = hkcr_PropSheet.write(nullptr, clsid_str);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// File type handler registered.
	return ERROR_SUCCESS;
}

/**
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ShellPropSheetExt::UnregisterCLSID(void)
{
	extern const wchar_t RP_ProgID[];

	// Unegister the COM object.
	LONG lResult = RegKey::UnregisterComObject(__uuidof(RP_ShellPropSheetExt), RP_ProgID);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// TODO
	return ERROR_SUCCESS;
}

/**
 * Unregister the file type handler.
 * @param hkcr HKEY_CLASSES_ROOT or user-specific classes root.
 * @param ext File extension, including the leading dot.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ShellPropSheetExt::UnregisterFileType(RegKey &hkcr, LPCWSTR ext)
{
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ShellPropSheetExt), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Open the file extension key.
	RegKey hkcr_ext(hkcr, ext, KEY_READ, false);
	if (!hkcr_ext.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		// In that case, it means we aren't registered.
		if (hkcr_ext.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_ext.lOpenRes();
	}

	// Unregister as a property sheet handler for this file association.

	// Open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkcr_ext, L"ShellEx", KEY_READ, false);
	if (!hkcr_ShellEx.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_ShellEx.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_ShellEx.lOpenRes();
	}

	// Open the "ShellEx\\PropertySheetHandlers" key.
	RegKey hkcr_PropSheetHandlers(hkcr_ShellEx, L"PropertySheetHandlers", KEY_READ, false);
	if (!hkcr_PropSheetHandlers.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_PropSheetHandlers.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_PropSheetHandlers.lOpenRes();
	}

	// Open the "rom-properties" property sheet handler key.
	RegKey hkcr_PropSheet(hkcr_PropSheetHandlers, RP_ProgID, KEY_READ, false);
	if (!hkcr_PropSheet.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_PropSheet.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_PropSheet.lOpenRes();
	}
	// Check if the default value matches the CLSID.
	wstring str_PropSheetCLSID = hkcr_PropSheet.read(nullptr);
	if (str_PropSheetCLSID == clsid_str) {
		// Default value matches.
		// Remove the subkey.
		hkcr_PropSheet.close();
		lResult = hkcr_PropSheetHandlers.deleteSubKey(RP_ProgID);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// Default value does not match.
		// We're done here.
		return ERROR_SUCCESS;
	}

	// Check if PropertySheetHandlers is empty.
	// TODO: Error handling.
	if (hkcr_PropSheetHandlers.isKeyEmpty()) {
		// No subkeys. Delete this key.
		hkcr_PropSheetHandlers.close();
		hkcr_ShellEx.deleteSubKey(L"PropertySheetHandlers");
	}

	// File type handler unregistered.
	return ERROR_SUCCESS;
}
