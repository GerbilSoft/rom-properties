/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ThumbnailProvider_Register.cpp: IThumbnailProvider implementation.   *
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
#include "RP_ThumbnailProvider.hpp"
#include "RP_ThumbnailProvider_p.hpp"

#include "RegKey.hpp"

// C++ includes.
#include <string>
using std::wstring;

#define IID_IThumbnailProvider_String		TEXT("{E357FCCD-A995-4576-B01F-234630154E96}")
#define CLSID_RP_ThumbnailProvider_String	TEXT("{4723DF58-463E-4590-8F4A-8D9DD4F4355A}")

/**
 * Register the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ThumbnailProvider::RegisterCLSID(void)
{
	static const wchar_t description[] = L"ROM Properties Page - Thumbnail Provider";
	extern const wchar_t RP_ProgID[];

	// Register the COM object.
	LONG lResult = RegKey::RegisterComObject(__uuidof(RP_ThumbnailProvider), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_ThumbnailProvider), description);
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
LONG RP_ThumbnailProvider_Private::RegisterFileType(RegKey &hkey_Assoc)
{
	// Register as the thumbnail handler for this file association.

	// Create/open the "ShellEx\\{IID_IThumbnailProvider}" key.
	// NOTE: This will recursively create the keys if necessary.
	RegKey hkcr_IThumbnailProvider(hkey_Assoc, L"ShellEx\\" IID_IThumbnailProvider_String, KEY_READ|KEY_WRITE, true);
	if (!hkcr_IThumbnailProvider.isOpen()) {
		return hkcr_IThumbnailProvider.lOpenRes();
	}

	// Is a custom IThumbnailProvider already registered?
	DWORD dwTypeTreatment;
	wstring clsid_reg = hkcr_IThumbnailProvider.read(nullptr);
	DWORD treatment = hkey_Assoc.read_dword(L"Treatment", &dwTypeTreatment);
	if (!clsid_reg.empty() && clsid_reg != CLSID_RP_ThumbnailProvider_String) {
		// Something else is registered.
		// Copy it to the fallback key.

		// FIXME: If an IExtractImage fallback interface is present
		// and IThumbnailProvider is not, or the IThumbnailProvider
		// class doesn't support IInitializeWithStream, don't register
		// the IThumbnailProvider interface. Windows Explorer won't
		// try the IExtractImage interface if IThumbnailProvider exists,
		// even if IThumbnailProvider fails.

		RegKey hkcr_RP_Fallback(hkey_Assoc, L"RP_Fallback", KEY_WRITE, true);
		if (!hkcr_RP_Fallback.isOpen()) {
			return hkcr_RP_Fallback.lOpenRes();
		}
		LONG lResult = hkcr_RP_Fallback.write(L"IThumbnailProvider", clsid_reg);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}

		if (dwTypeTreatment == REG_DWORD) {
			// Copy the treatment value.
			lResult = hkcr_RP_Fallback.write_dword(L"Treatment", treatment);
			if (lResult != ERROR_SUCCESS) {
				return lResult;
			}
		} else {
			// Delete the Treatment value if it's there.
			lResult = hkcr_RP_Fallback.deleteValue(L"Treatment");
			if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
				return lResult;
			}
		}
	}

	// NOTE: We're not skipping this even if the CLSID
	// is correct, just in case some setting needs to
	// be refreshed.

	// Set the IThumbnailProvider to this CLSID.
	LONG lResult = hkcr_IThumbnailProvider.write(nullptr, CLSID_RP_ThumbnailProvider_String);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Set the "Treatment" value.
	lResult = hkey_Assoc.write_dword(L"Treatment", 0);
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
LONG RP_ThumbnailProvider::RegisterFileType(RegKey &hkcr, LPCWSTR ext)
{
	// Open the file extension key.
	RegKey hkcr_ext(hkcr, ext, KEY_READ|KEY_WRITE, true);
	if (!hkcr_ext.isOpen()) {
		return hkcr_ext.lOpenRes();
	}

	// Register the main association.
	LONG lResult = RP_ThumbnailProvider_Private::RegisterFileType(hkcr_ext);
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
		lResult = RP_ThumbnailProvider_Private::RegisterFileType(hkcr_ProgID);
	}

	// File type handler registered.
	return lResult;
}

/**
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ThumbnailProvider::UnregisterCLSID(void)
{
	extern const wchar_t RP_ProgID[];

	// Unegister the COM object.
	LONG lResult = RegKey::UnregisterComObject(__uuidof(RP_ThumbnailProvider), RP_ProgID);
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
LONG RP_ThumbnailProvider_Private::UnregisterFileType(RegKey &hkey_Assoc)
{
	// Unregister as the thumbnail handler for this file association.

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

	// Open the {IID_IThumbnailProvider} key.
	RegKey hkcr_IThumbnailProvider(hkcr_ShellEx, IID_IThumbnailProvider_String, KEY_READ|KEY_WRITE, false);
	if (!hkcr_IThumbnailProvider.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		LONG lResult = hkcr_IThumbnailProvider.lOpenRes();
		if (lResult == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return lResult;
	}

	// Check if the default value matches the CLSID.
	wstring wstr_IThumbnailProvider = hkcr_IThumbnailProvider.read(nullptr);
	if (wstr_IThumbnailProvider != CLSID_RP_ThumbnailProvider_String) {
		// Not our IThumbnailProvider.
		// We're done here.
		return ERROR_SUCCESS;
	}

	// Restore the fallbacks if we have any.
	wstring clsid_reg;
	DWORD dwTypeTreatment = 0;
	DWORD treatment = 0;
	RegKey hkcr_RP_Fallback(hkey_Assoc, L"RP_Fallback", KEY_READ|KEY_WRITE, false);
	if (hkcr_RP_Fallback.isOpen()) {
		// Read the fallbacks.
		clsid_reg = hkcr_RP_Fallback.read(L"IThumbnailProvider");
		treatment = hkcr_RP_Fallback.read_dword(L"Treatment", &dwTypeTreatment);
	}

	if (!clsid_reg.empty()) {
		// Restore the IThumbnailProvider.
		LONG lResult = hkcr_IThumbnailProvider.write(nullptr, clsid_reg);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
		// Restore the "Treatment" value.
		if (dwTypeTreatment == REG_DWORD) {
			lResult = hkey_Assoc.write_dword(L"Treatment", treatment);
			if (lResult != ERROR_SUCCESS) {
				return lResult;
			}
		} else {
			// No "Treatment" value to restore.
			// Delete the current one if it's present.
			lResult = hkey_Assoc.deleteValue(L"Treatment");
			if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
				return lResult;
			}
		}
	} else {
		// No IThumbnailProvider to restore.
		// Remove the current one.
		hkcr_IThumbnailProvider.close();
		LONG lResult = hkcr_ShellEx.deleteSubKey(IID_IThumbnailProvider_String);
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
		}

		// Remove the "Treatment" value if it's present.
		lResult = hkey_Assoc.deleteValue(L"Treatment");
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
		}
	}

	// Remove the fallbacks.
	LONG lResult = ERROR_SUCCESS;
	if (hkcr_RP_Fallback.isOpen()) {
		lResult = hkcr_RP_Fallback.deleteValue(L"IThumbnailProvider");
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
		}
		lResult = hkcr_RP_Fallback.deleteValue(L"Treatment");
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
LONG RP_ThumbnailProvider::UnregisterFileType(RegKey &hkcr, LPCWSTR ext)
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
	LONG lResult = RP_ThumbnailProvider_Private::UnregisterFileType(hkcr_ext);
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
		lResult = RP_ThumbnailProvider_Private::UnregisterFileType(hkcr_ProgID);
	}

	// File type handler unregistered.
	return lResult;
}
