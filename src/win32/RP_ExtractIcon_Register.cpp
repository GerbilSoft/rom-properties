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
#include "RP_ExtractIcon_p.hpp"

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
 *
 * Internal version; this only registers for a single Classes key.
 * Called by the public version multiple times if a ProgID is registered.
 *
 * @param hkey_Assoc File association key to register under.
 * @param progID If true, don't set DefaultIcon if it's empty. (ProgID mode)
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon_Private::RegisterFileType(RegKey &hkey_Assoc, bool progID_mode)
{
	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ExtractIcon), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Register as the icon handler for this file association.

	// Create/open the "DefaultIcon" key.
	RegKey hkcr_DefaultIcon(hkey_Assoc, L"DefaultIcon", KEY_READ|KEY_WRITE, !progID_mode);
	if (!hkcr_DefaultIcon.isOpen()) {
		if (progID_mode && lResult == ERROR_FILE_NOT_FOUND) {
			// No DefaultIcon.
			return ERROR_SUCCESS;
		}
		return hkcr_DefaultIcon.lOpenRes();
	}

	// Create/open the "ShellEx\\IconHandler" key.
	RegKey hkcr_IconHandler(hkey_Assoc, L"ShellEx\\IconHandler", KEY_READ|KEY_WRITE, true);
	if (!hkcr_IconHandler.isOpen()) {
		return hkcr_IconHandler.lOpenRes();
	}

	// Is a custom IconHandler already registered?
	DWORD dwTypeDefaultIcon, dwTypeIconHandler;
	wstring defaultIcon = hkcr_DefaultIcon.read(nullptr, &dwTypeDefaultIcon);
	wstring iconHandler = hkcr_IconHandler.read(nullptr, &dwTypeIconHandler);
	if (progID_mode && defaultIcon.empty()) {
		// ProgID mode, and no default icon.
		// Nothing to do here.
		return ERROR_SUCCESS;
	} else if (defaultIcon == L"%1") {
		// "%1" == use IconHandler
		if (iconHandler != clsid_str) {
			// Something else is registered.
			// Copy it to the fallback key.
			RegKey hkcr_RP_Fallback(hkey_Assoc, L"RP_Fallback", KEY_WRITE, true);
			if (!hkcr_RP_Fallback.isOpen()) {
				return hkcr_RP_Fallback.lOpenRes();
			}
			lResult = hkcr_RP_Fallback.write(L"DefaultIcon", defaultIcon, dwTypeDefaultIcon);
			if (lResult != ERROR_SUCCESS) {
				return lResult;
			}
			lResult = hkcr_RP_Fallback.write(L"IconHandler", iconHandler, dwTypeIconHandler);
			if (lResult != ERROR_SUCCESS) {
				return lResult;
			}
		}
	} else {
		// Not an icon handler.
		// Copy it to the fallback key.
		RegKey hkcr_RP_Fallback(hkey_Assoc, L"RP_Fallback", KEY_WRITE, true);
		if (!hkcr_RP_Fallback.isOpen()) {
			return hkcr_RP_Fallback.lOpenRes();
		}
		if (defaultIcon.empty()) {
			// No DefaultIcon.
			// Delete it if it's present.
			lResult = hkcr_RP_Fallback.deleteValue(L"DefaultIcon");
			if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
				return lResult;
			}
		} else {
			// Save the DefaultIcon.
			lResult = hkcr_RP_Fallback.write(L"DefaultIcon", defaultIcon, dwTypeDefaultIcon);
			if (lResult != ERROR_SUCCESS) {
				return lResult;
			}
		}

		// Delete IconHandler if it's present.
		lResult = hkcr_RP_Fallback.deleteValue(L"IconHandler");
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
		}
	}

	// Set the IconHandler to this CLSID.
	lResult = hkcr_IconHandler.write(nullptr, clsid_str);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Set the DefaultIcon to "%1".
	lResult = hkcr_DefaultIcon.write(nullptr, L"%1");

	// File type handler registered.
	return lResult;
}

/**
 * Register the file type handler.
 * @param hkcr HKEY_CLASSES_ROOT or user-specific classes root.
 * @param ext File extension, including the leading dot.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon::RegisterFileType(RegKey &hkcr, LPCWSTR ext)
{
	// Open the file extension key.
	RegKey hkcr_ext(HKEY_CLASSES_ROOT, ext, KEY_READ|KEY_WRITE, true);
	if (!hkcr_ext.isOpen()) {
		return hkcr_ext.lOpenRes();
	}

	// Register the main association.
	LONG lResult = RP_ExtractIcon_Private::RegisterFileType(hkcr_ext, false);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Is a custom ProgID registered?
	// If so, and it has a DefaultIcon registered,
	// we'll need to update the custom ProgID.
	wstring progID = hkcr_ext.read(nullptr);
	if (!progID.empty()) {
		// Custom ProgID is registered.
		RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, progID.c_str(), KEY_READ|KEY_WRITE, false);
		if (!hkcr_ProgID.isOpen()) {
			lResult = hkcr_ProgID.lOpenRes();
			if (lResult == ERROR_FILE_NOT_FOUND) {
				// ProgID not found. This is okay.
				return ERROR_SUCCESS;
			} else {
				return hkcr_ProgID.lOpenRes();
			}
		}
		lResult = RP_ExtractIcon_Private::RegisterFileType(hkcr_ProgID, true);
	}

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
 *
 * Internal version; this only unregisters for a single Classes key.
 * Called by the public version multiple times if a ProgID is registered.
 *
 * @param hkey_Assoc File association key to unregister under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon_Private::UnregisterFileType(RegKey &hkey_Assoc)
{
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ExtractIcon), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0) {
		return ERROR_INVALID_PARAMETER;
	}

	// Unregister as the icon handler for this file association.

	// Open the "DefaultIcon" key.
	RegKey hkcr_DefaultIcon(hkey_Assoc, L"DefaultIcon", KEY_READ|KEY_WRITE, false);
	if (!hkcr_DefaultIcon.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		// In that case, it means we aren't registered.
		if (hkcr_DefaultIcon.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_DefaultIcon.lOpenRes();
	}

	// Open the "ShellEx\\IconHandler" key.
	RegKey hkcr_IconHandler(hkey_Assoc, L"ShellEx\\IconHandler", KEY_READ|KEY_WRITE, false);
	if (!hkcr_IconHandler.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		// In that case, it means we aren't registered.
		if (hkcr_IconHandler.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_IconHandler.lOpenRes();
	}

	// Check if DefaultIcon is "%1" and IconHandler is our CLSID.
	wstring defaultIcon = hkcr_DefaultIcon.read(nullptr);
	wstring iconHandler = hkcr_IconHandler.read(nullptr);
	if (defaultIcon != L"%1" || iconHandler != clsid_str) {
		// Not our DefaultIcon or IconHandler.
		// We're done here.
		return ERROR_SUCCESS;
	}

	// Restore the fallbacks if we have any.
	DWORD dwTypeDefaultIcon, dwTypeIconHandler;
	defaultIcon.clear();
	iconHandler.clear();
	RegKey hkcr_RP_Fallback(hkey_Assoc, L"RP_Fallback", KEY_READ|KEY_WRITE, false);
	if (hkcr_RP_Fallback.isOpen()) {
		// Read the fallbacks.
		defaultIcon = hkcr_RP_Fallback.read(L"DefaultIcon", &dwTypeDefaultIcon);
		iconHandler = hkcr_RP_Fallback.read(L"IconHandler", &dwTypeIconHandler);
	}

	if (!defaultIcon.empty()) {
		// Restore the DefaultIcon.
		lResult = hkcr_DefaultIcon.write(nullptr, defaultIcon, dwTypeDefaultIcon);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// Delete the DefaultIcon.
		hkcr_DefaultIcon.close();
		hkey_Assoc.deleteSubKey(L"DefaultIcon");
	}

	if (!iconHandler.empty()) {
		// Restore the IconHandler.
		lResult = hkcr_DefaultIcon.write(nullptr, iconHandler, dwTypeIconHandler);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// Delete the IconHandler.
		hkcr_IconHandler.close();
		hkey_Assoc.deleteSubKey(L"IconHandler");
	}

	// Remove the fallbacks.
	lResult = hkcr_RP_Fallback.deleteValue(L"DefaultIcon");
	if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
		return lResult;
	}
	lResult = hkcr_RP_Fallback.deleteValue(L"IconHandler");
	if (lResult == ERROR_FILE_NOT_FOUND) {
		lResult = ERROR_SUCCESS;
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
LONG RP_ExtractIcon::UnregisterFileType(RegKey &hkcr, LPCWSTR ext)
{
	// Open the file extension key.
	RegKey hkcr_ext(HKEY_CLASSES_ROOT, ext, KEY_READ|KEY_WRITE, true);
	if (!hkcr_ext.isOpen()) {
		return hkcr_ext.lOpenRes();
	}

	// Unregister the main association.
	LONG lResult = RP_ExtractIcon_Private::UnregisterFileType(hkcr_ext);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Is a custom ProgID registered?
	// If so, and it has a DefaultIcon registered,
	// we'll need to update the custom ProgID.
	wstring progID = hkcr_ext.read(nullptr);
	if (!progID.empty()) {
		// Custom ProgID is registered.
		RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, progID.c_str(), KEY_READ|KEY_WRITE, false);
		if (!hkcr_ProgID.isOpen()) {
			lResult = hkcr_ProgID.lOpenRes();
			if (lResult == ERROR_FILE_NOT_FOUND) {
				// ProgID not found. This is okay.
				return ERROR_SUCCESS;
			} else {
				return hkcr_ProgID.lOpenRes();
			}
		}
		lResult = RP_ExtractIcon_Private::UnregisterFileType(hkcr_ProgID);
	}

	// File type handler unregistered.
	return lResult;
}
