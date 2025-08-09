/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon_Register.cpp: IExtractIcon implementation.               *
 * COM registration functions.                                             *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RP_ExtractIcon.hpp"
#include "RP_ExtractIcon_p.hpp"

// libwin32common
using LibWin32UI::RegKey;

// C++ STL classes.
using std::tstring;

#define CLSID_RP_ExtractIcon_String	TEXT("{E51BC107-E491-4B29-A6A3-2A4309259802}")

/**
 * Register the file type handler.
 *
 * Internal version; this only registers for a single Classes key.
 * Called by the public version multiple times if a ProgID is registered.
 *
 * @param hkey_Assoc File association key to register under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon_Private::RegisterFileType(RegKey &hkey_Assoc)
{
	// Register as the icon handler for this file association.

	// Create/open the "DefaultIcon" key.
	RegKey hkcr_DefaultIcon(hkey_Assoc, _T("DefaultIcon"), KEY_READ|KEY_WRITE, true);
	if (!hkcr_DefaultIcon.isOpen()) {
		return hkcr_DefaultIcon.lOpenRes();
	}

	// Create/open the "ShellEx\\IconHandler" key.
	// NOTE: This will recursively create the keys if necessary.
	RegKey hkcr_IconHandler(hkey_Assoc, _T("ShellEx\\IconHandler"), KEY_READ|KEY_WRITE, true);
	if (!hkcr_IconHandler.isOpen()) {
		return hkcr_IconHandler.lOpenRes();
	}

	// Is a custom IconHandler already registered?
	DWORD dwTypeDefaultIcon, dwTypeIconHandler;
	const tstring defaultIcon = hkcr_DefaultIcon.read(nullptr, &dwTypeDefaultIcon);
	const tstring iconHandler = hkcr_IconHandler.read(nullptr, &dwTypeIconHandler);
	if (defaultIcon == _T("%1")) {
		// "%1" == use IconHandler
		if (dwTypeIconHandler != 0 && iconHandler != CLSID_RP_ExtractIcon_String) {
			// Something else is registered.
			// Copy it to the fallback key.
			RegKey hkcr_RP_Fallback(hkey_Assoc, _T("RP_Fallback"), KEY_WRITE, true);
			if (!hkcr_RP_Fallback.isOpen()) {
				return hkcr_RP_Fallback.lOpenRes();
			}
			LONG lResult = hkcr_RP_Fallback.write(_T("DefaultIcon"), defaultIcon, dwTypeDefaultIcon);
			if (lResult != ERROR_SUCCESS) {
				return lResult;
			}
			lResult = hkcr_RP_Fallback.write(_T("IconHandler"), iconHandler, dwTypeIconHandler);
			if (lResult != ERROR_SUCCESS) {
				return lResult;
			}
		}
	} else if (!defaultIcon.empty()) {
		// Not an icon handler.
		// Copy it to the fallback key.
		RegKey hkcr_RP_Fallback(hkey_Assoc, _T("RP_Fallback"), KEY_WRITE, true);
		if (!hkcr_RP_Fallback.isOpen()) {
			return hkcr_RP_Fallback.lOpenRes();
		}

		LONG lResult;
		if (defaultIcon.empty()) {
			// No DefaultIcon.
			// Delete it if it's present.
			lResult = hkcr_RP_Fallback.deleteValue(_T("DefaultIcon"));
			if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
				return lResult;
			}
		} else {
			// Save the DefaultIcon.
			lResult = hkcr_RP_Fallback.write(_T("DefaultIcon"), defaultIcon, dwTypeDefaultIcon);
			if (lResult != ERROR_SUCCESS) {
				return lResult;
			}
		}

		// Delete IconHandler if it's present.
		lResult = hkcr_RP_Fallback.deleteValue(_T("IconHandler"));
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
		}
	}

	// NOTE: We're not skipping this even if the IconHandler
	// is correct, just in case some setting needs to
	// be refreshed.

	// Set the IconHandler to this CLSID.
	LONG lResult = hkcr_IconHandler.write(nullptr, CLSID_RP_ExtractIcon_String);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Set the DefaultIcon to "%1".
	lResult = hkcr_DefaultIcon.write(nullptr, _T("%1"));

	// File type handler registered.
	return lResult;
}

/**
 * Register the file type handler.
 * @param hkcr	[in] HKEY_CLASSES_ROOT or user-specific classes root.
 * @param ext	[in] File extension, including the leading dot.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon::RegisterFileType(_In_ RegKey &hkcr, _In_ LPCTSTR ext)
{
	// Open the file extension key.
	RegKey hkcr_ext(hkcr, ext, KEY_READ|KEY_WRITE, true);
	if (!hkcr_ext.isOpen()) {
		return hkcr_ext.lOpenRes();
	}

	// Register the main association.
	LONG lResult = RP_ExtractIcon_Private::RegisterFileType(hkcr_ext);
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
		lResult = RP_ExtractIcon_Private::RegisterFileType(hkcr_ProgID);
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
LONG RP_ExtractIcon_Private::UnregisterFileType(RegKey &hkey_Assoc)
{
	// Unregister as the icon handler for this file association.
	// NOTE: Continuing even if some keys are missing in case there
	// are other leftover keys.

	// Open the "DefaultIcon" key.
	tstring defaultIcon;
	RegKey hkcr_DefaultIcon(hkey_Assoc, _T("DefaultIcon"), KEY_READ|KEY_WRITE, false);
	if (hkcr_DefaultIcon.isOpen()) {
		defaultIcon = hkcr_DefaultIcon.read(nullptr);
	} else {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		// In that case, it means we aren't registered.
		LONG lResult = hkcr_DefaultIcon.lOpenRes();
		if (lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
		}
	}

	// Open the "ShellEx\\IconHandler" key.
	tstring iconHandler;
	RegKey hkcr_IconHandler(hkey_Assoc, _T("ShellEx\\IconHandler"), KEY_READ|KEY_WRITE, false);
	if (hkcr_IconHandler.isOpen()) {
		iconHandler = hkcr_IconHandler.read(nullptr);
	} else {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		// In that case, it means we aren't registered.
		LONG lResult = hkcr_DefaultIcon.lOpenRes();
		if (lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
		}
	}

	// Check if DefaultIcon is "%1" and IconHandler is our CLSID.
	// FIXME: Restore if iconHandler matches, even if defaultIcon doesn't.
	if (defaultIcon != _T("%1")) {
		// Not our DefaultIcon.
		hkcr_DefaultIcon.close();
	}
	if (iconHandler != CLSID_RP_ExtractIcon_String) {
		// Not our IconHandler.
		hkcr_DefaultIcon.close();
		hkcr_IconHandler.close();
	}

	// Restore the fallbacks if we have any.
	DWORD dwTypeDefaultIcon = 0, dwTypeIconHandler = 0;
	tstring defaultIcon_fb, iconHandler_fb;
	RegKey hkcr_RP_Fallback(hkey_Assoc, _T("RP_Fallback"), KEY_READ|KEY_WRITE, false);
	if (hkcr_RP_Fallback.isOpen()) {
		// Read the fallbacks.
		defaultIcon_fb = hkcr_RP_Fallback.read(_T("DefaultIcon"), &dwTypeDefaultIcon);
		iconHandler_fb = hkcr_RP_Fallback.read(_T("IconHandler"), &dwTypeIconHandler);
	}

	if (hkcr_DefaultIcon.isOpen()) {
		if (!defaultIcon_fb.empty()) {
			// Restore the DefaultIcon.
			LONG lResult = hkcr_DefaultIcon.write(nullptr, defaultIcon_fb, dwTypeDefaultIcon);
			if (lResult != ERROR_SUCCESS) {
				return lResult;
			}
		} else {
			// Delete the DefaultIcon.
			hkcr_DefaultIcon.close();
			LONG lResult = hkey_Assoc.deleteSubKey(_T("DefaultIcon"));
			if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
				return lResult;
			}
		}
	}

	if (hkcr_IconHandler.isOpen()) {
		if (!iconHandler_fb.empty()) {
			// Restore the IconHandler.
			LONG lResult = hkcr_IconHandler.write(nullptr, iconHandler_fb, dwTypeIconHandler);
			if (lResult != ERROR_SUCCESS) {
				return lResult;
			}
		} else {
			// Open the "ShellEx" key.
			RegKey hkcr_ShellEx(hkey_Assoc, _T("ShellEx"), KEY_READ|KEY_WRITE, false);
			if (hkcr_ShellEx.isOpen()) {
				// Delete the IconHandler.
				// FIXME: Windows 7 isn't properly deleting this in some cases...
				// (".3gp" extension; owned by WMP11)
				hkcr_IconHandler.close();
				LONG lResult = hkcr_ShellEx.deleteSubKey(_T("IconHandler"));
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
	}

	// Remove the fallbacks.
	if (hkcr_RP_Fallback.isOpen()) {
		LONG lResult = hkcr_RP_Fallback.deleteValue(_T("DefaultIcon"));
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
		}
		lResult = hkcr_RP_Fallback.deleteValue(_T("IconHandler"));
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
LONG RP_ExtractIcon::UnregisterFileType(_In_ RegKey &hkcr, _In_opt_ LPCTSTR ext)
{
	if (!ext) {
		// Unregister from hkcr directly.
		return RP_ExtractIcon_Private::UnregisterFileType(hkcr);
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
	LONG lResult = RP_ExtractIcon_Private::UnregisterFileType(hkcr_ext);
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
		lResult = RP_ExtractIcon_Private::UnregisterFileType(hkcr_ProgID);
	}

	// File type handler unregistered.
	return lResult;
}
