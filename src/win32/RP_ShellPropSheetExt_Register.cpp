/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt_Register.cpp: IShellPropSheetExt implementation.   *
 * COM registration functions.                                             *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RP_ShellPropSheetExt.hpp"

// libwin32common
using LibWin32UI::RegKey;

// C++ STL classes.
using std::tstring;

#define CLSID_RP_ShellPropSheetExt_String	TEXT("{2443C158-DF7C-4352-B435-BC9F885FFD52}")
CLSID_IMPL(RP_ShellPropSheetExt, _T("ROM Properties Page - Property Sheet"))

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
LONG RP_ShellPropSheetExt::RegisterFileType_int(RegKey &hkey_Assoc)
{
	// Register as a property sheet handler for this file association.

	// Create/open the "ShellEx\\PropertySheetHandlers\\rom-properties" key.
	// NOTE: This will recursively create the keys if necessary.
	tstring keyname = _T("ShellEx\\PropertySheetHandlers\\");
	keyname += RP_ProgID;
	RegKey hkcr_PropSheet(hkey_Assoc, keyname.c_str(), KEY_WRITE, true);
	if (!hkcr_PropSheet.isOpen()) {
		return hkcr_PropSheet.lOpenRes();
	}

	// Set the default value to this CLSID.
	LONG lResult = hkcr_PropSheet.write(nullptr, CLSID_RP_ShellPropSheetExt_String);
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
LONG RP_ShellPropSheetExt::RegisterFileType(_In_ RegKey &hkcr, _In_ LPCTSTR ext)
{
	// Open the file extension key.
	RegKey hkcr_ext(hkcr, ext, KEY_READ|KEY_WRITE, true);
	if (!hkcr_ext.isOpen()) {
		return hkcr_ext.lOpenRes();
	}

	// Register the main association.
	LONG lResult = RegisterFileType_int(hkcr_ext);
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
		lResult = RegisterFileType_int(hkcr_ProgID);
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
LONG RP_ShellPropSheetExt::UnregisterFileType_int(RegKey &hkey_Assoc)
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

	// Open the "ShellEx\\PropertySheetHandlers" key.
	RegKey hkcr_PropSheetHandlers(hkcr_ShellEx, _T("PropertySheetHandlers"), KEY_READ, false);
	if (!hkcr_PropSheetHandlers.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		LONG lResult = hkcr_PropSheetHandlers.lOpenRes();
		if (lResult == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return lResult;
	}

	// Open the "rom-properties" property sheet handler key.
	RegKey hkcr_PropSheet(hkcr_PropSheetHandlers, RP_ProgID, KEY_READ, false);
	if (!hkcr_PropSheet.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		LONG lResult = hkcr_PropSheet.lOpenRes();
		if (lResult == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return lResult;
	}
	// Check if the default value matches the CLSID.
	const tstring str_PropSheetCLSID = hkcr_PropSheet.read(nullptr);
	if (str_PropSheetCLSID == CLSID_RP_ShellPropSheetExt_String) {
		// Default value matches.
		// Remove the subkey.
		hkcr_PropSheet.close();
		LONG lResult = hkcr_PropSheetHandlers.deleteSubKey(RP_ProgID);
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
		LONG lResult = hkcr_ShellEx.deleteSubKey(_T("PropertySheetHandlers"));
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
LONG RP_ShellPropSheetExt::UnregisterFileType(_In_ RegKey &hkcr, _In_opt_ LPCTSTR ext)
{
	// NOTE: NULL ext isn't needed for RP_ShellPropSheetExt.
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
	LONG lResult = UnregisterFileType_int(hkcr_ext);
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
		lResult = UnregisterFileType_int(hkcr_ProgID);
	}

	// File type handler unregistered.
	return lResult;
}
