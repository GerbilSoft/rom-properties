/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ColumnProvider_Register.cpp: IColumnProvider implementation.         *
 * COM registration functions.                                             *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RP_ColumnProvider.hpp"
//#include "RP_ColumnProvider_p.hpp"

// libwin32common
using LibWin32UI::RegKey;

// C++ STL classes
using std::tstring;
using std::unique_ptr;

#define CLSID_RP_ColumnProvider_String	TEXT("{126621F9-01E7-45DA-BC4F-CBDFAB9C0E0A}")

extern const TCHAR RP_ProgID[];

/**
 * Register the file type handler.
 *
 * Internal version; this only registers for a single Classes key.
 * Called by the public version multiple times if a ProgID is registered.
 *
 * @param hkey_Assoc File association key to register under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ColumnProvider/*_Private*/::RegisterFileType_int(RegKey &hkey_Assoc)
{
	// Register as a column handler for this file association.

	// Create/open the "ShellEx\\ColumnHandlers\\{CLSID}" key.
	// NOTE: This will recursively create the keys if necessary.
	tstring keyname = _T("ShellEx\\ColumnHandlers\\");
	keyname += CLSID_RP_ColumnProvider_String;
	RegKey hkcr_RpColumnProvider(hkey_Assoc, keyname.c_str(), KEY_WRITE, true);
	if (!hkcr_RpColumnProvider.isOpen()) {
		return hkcr_RpColumnProvider.lOpenRes();
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
LONG RP_ColumnProvider::RegisterFileType(_In_ RegKey &hkcr, _In_ LPCTSTR ext)
{
	// Open the file extension key.
	RegKey hkcr_ext(hkcr, ext, KEY_READ|KEY_WRITE, true);
	if (!hkcr_ext.isOpen()) {
		return hkcr_ext.lOpenRes();
	}

	// Register the main association.
	LONG lResult = /*RP_ColumnProvider_Private::*/ RegisterFileType_int(hkcr_ext);
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
		lResult = /*RP_ColumnProvider_Private::*/RegisterFileType_int(hkcr_ProgID);
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
LONG RP_ColumnProvider/*_Private*/::UnregisterFileType_int(RegKey &hkey_Assoc)
{
	// Unregister as a property sheet handler for this file association.

	// Open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkey_Assoc, _T("ShellEx"), KEY_READ, false);
	if (!hkcr_ShellEx.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		LONG lResult = hkcr_ShellEx.lOpenRes();
		if (lResult == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return lResult;
	}

	// Open the "ShellEx\\ColumnHandlers" key.
	RegKey hkcr_ColumnHandlers(hkcr_ShellEx, _T("ColumnHandlers"), KEY_READ, false);
	if (!hkcr_ColumnHandlers.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		LONG lResult = hkcr_ColumnHandlers.lOpenRes();
		if (lResult == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return lResult;
	}

	// Delete the subkey with our CLSID. (no error checking)
	hkcr_ColumnHandlers.deleteSubKey(CLSID_RP_ColumnProvider_String);

	// Check if ColumnHandlers is empty.
	// TODO: Error handling.
	if (hkcr_ColumnHandlers.isKeyEmpty()) {
		// No subkeys. Delete this key.
		hkcr_ColumnHandlers.close();
		LONG lResult = hkcr_ShellEx.deleteSubKey(_T("ColumnHandlers"));
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
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
LONG RP_ColumnProvider::UnregisterFileType(_In_ RegKey &hkcr, _In_opt_ LPCTSTR ext)
{
	// NOTE: NULL ext isn't needed for RP_ColumnProvider.
	assert(ext != nullptr);
	if (!ext) {
		return ERROR_FILE_NOT_FOUND;
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
	LONG lResult = /*RP_ColumnProvider_Private::*/UnregisterFileType_int(hkcr_ext);
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
		lResult = /*RP_ColumnProvider_Private::*/UnregisterFileType_int(hkcr_ProgID);
	}

	// File type handler unregistered.
	return lResult;
}
