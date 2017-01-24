/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon_Register.cpp: IExtractIcon implementation.               *
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
#include "RP_ExtractIcon.hpp"
#include "RegKey.hpp"

// C++ includes.
#include <string>
using std::wstring;

/**
 * Register the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon::RegisterCLSID(void)
{
	static const wchar_t description[] = L"ROM Properties Page - Icon Extractor";
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ExtractIcon), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Register the COM object.
	lResult = RegKey::RegisterComObject(__uuidof(RP_ExtractIcon), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_ExtractIcon), description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// COM object registered.
	return ERROR_SUCCESS;
}

/**
 * Register the file type handler.
 * @param hkey_Assoc File association key to register under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon::RegisterFileType(RegKey &hkey_Assoc)
{
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ExtractIcon), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Register as the icon handler for this file association.

	// Create/open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkey_Assoc, L"ShellEx", KEY_WRITE, true);
	if (!hkcr_ShellEx.isOpen()) {
		return hkcr_ShellEx.lOpenRes();
	}

	// Create/open the "IconHandler" key.
	RegKey hkcr_IconHandler(hkcr_ShellEx, L"IconHandler", KEY_WRITE, true);
	if (!hkcr_IconHandler.isOpen()) {
		return hkcr_IconHandler.lOpenRes();
	}
	// Set the default value to this CLSID.
	lResult = hkcr_IconHandler.write(nullptr, clsid_str);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Create/open the "DefaultIcon" key.
	RegKey hkcr_DefaultIcon(hkey_Assoc, L"DefaultIcon", KEY_WRITE, true);
	if (!hkcr_DefaultIcon.isOpen()) {
		return hkcr_DefaultIcon.lOpenRes();
	}
	// Set the default value to "%1".
	lResult = hkcr_DefaultIcon.write(nullptr, L"%1");

	// File type handler registered.
	return lResult;
}

/**
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon::UnregisterCLSID(void)
{
	extern const wchar_t RP_ProgID[];

	// Unegister the COM object.
	return RegKey::UnregisterComObject(__uuidof(RP_ExtractIcon), RP_ProgID);
}

/**
 * Unregister the file type handler.
 * @param hkey_Assoc File association key to register under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon::UnregisterFileType(RegKey &hkey_Assoc)
{
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ExtractIcon), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Unregister as the icon handler for this file association.

	// Open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkey_Assoc, L"ShellEx", KEY_WRITE, false);
	if (!hkcr_ShellEx.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_ShellEx.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_ShellEx.lOpenRes();
	}

	// Open the "IconHandler" key.
	RegKey hkcr_IconHandler(hkcr_ShellEx, L"IconHandler", KEY_READ, false);
	if (!hkcr_IconHandler.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_IconHandler.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_IconHandler.lOpenRes();
	}
	// Check if the default value matches the CLSID.
	wstring str_IconHandler = hkcr_IconHandler.read(nullptr);
	if (str_IconHandler == clsid_str) {
		// Default value matches.
		// Remove the subkey.
		hkcr_IconHandler.close();
		lResult = hkcr_ShellEx.deleteSubKey(L"IconHandler");
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// Default value does not match.
		// We're done here.
		return hkcr_IconHandler.lOpenRes();
	}

	// Open the "DefaultIcon" key.
	RegKey hkcr_DefaultIcon(hkey_Assoc, L"DefaultIcon", KEY_READ, false);
	if (!hkcr_DefaultIcon.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkcr_DefaultIcon.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_DefaultIcon.lOpenRes();
	}
	// Check if the default value is "%1".
	wstring wstr_DefaultIcon = hkcr_DefaultIcon.read(nullptr);
	if (wstr_DefaultIcon == L"%1") {
		// Default value matches.
		// Remove the subkey.
		hkcr_DefaultIcon.close();
		lResult = hkey_Assoc.deleteSubKey(L"DefaultIcon");
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// Default value doesn't match.
		// We're done here.
		return hkcr_DefaultIcon.lOpenRes();
	}

	// File type handler unregistered.
	return ERROR_SUCCESS;
}
