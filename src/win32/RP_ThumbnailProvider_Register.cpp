/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ThumbnailProvider_Register.cpp: IThumbnailProvider implementation.   *
 * COM registration functions.                                             *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RP_ThumbnailProvider.hpp"
#include "RP_ThumbnailProvider_p.hpp"

// libwin32common
using LibWin32UI::RegKey;

// C++ STL classes.
using std::tstring;
using std::unique_ptr;

#define IID_IThumbnailProvider_String		TEXT("{E357FCCD-A995-4576-B01F-234630154E96}")
#define CLSID_RP_ThumbnailProvider_String	TEXT("{4723DF58-463E-4590-8F4A-8D9DD4F4355A}")
CLSID_IMPL(RP_ThumbnailProvider, _T("ROM Properties Page - Thumbnail Provider"))

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
	RegKey hkcr_IThumbnailProvider(hkey_Assoc, _T("ShellEx\\" IID_IThumbnailProvider_String), KEY_READ|KEY_WRITE, true);
	if (!hkcr_IThumbnailProvider.isOpen()) {
		return hkcr_IThumbnailProvider.lOpenRes();
	}

	// Is a custom IThumbnailProvider already registered?
	DWORD dwTypeTreatment;
	const tstring clsid_reg = hkcr_IThumbnailProvider.read(nullptr);
	DWORD treatment = hkey_Assoc.read_dword(_T("Treatment"), &dwTypeTreatment);
	if (!clsid_reg.empty() && clsid_reg != CLSID_RP_ThumbnailProvider_String) {
		// Something else is registered.
		// Copy it to the fallback key.

		// FIXME: If an IExtractImage fallback interface is present
		// and IThumbnailProvider is not, or the IThumbnailProvider
		// class doesn't support IInitializeWithStream, don't register
		// the IThumbnailProvider interface. Windows Explorer won't
		// try the IExtractImage interface if IThumbnailProvider exists,
		// even if IThumbnailProvider fails.

		RegKey hkcr_RP_Fallback(hkey_Assoc, _T("RP_Fallback"), KEY_WRITE, true);
		if (!hkcr_RP_Fallback.isOpen()) {
			return hkcr_RP_Fallback.lOpenRes();
		}
		LONG lResult = hkcr_RP_Fallback.write(_T("IThumbnailProvider"), clsid_reg);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}

		if (dwTypeTreatment == REG_DWORD) {
			// Copy the treatment value.
			lResult = hkcr_RP_Fallback.write_dword(_T("Treatment"), treatment);
			if (lResult != ERROR_SUCCESS) {
				return lResult;
			}
		} else {
			// Delete the Treatment value if it's there.
			lResult = hkcr_RP_Fallback.deleteValue(_T("Treatment"));
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
	lResult = hkey_Assoc.write_dword(_T("Treatment"), 0);
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
LONG RP_ThumbnailProvider::RegisterFileType(_In_ RegKey &hkcr, _In_ LPCTSTR ext)
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
		lResult = RP_ThumbnailProvider_Private::RegisterFileType(hkcr_ProgID);
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
LONG RP_ThumbnailProvider_Private::UnregisterFileType(RegKey &hkey_Assoc)
{
	// Unregister as the thumbnail handler for this file association.
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

	// Open the {IID_IThumbnailProvider} key.
	unique_ptr<RegKey> phkcr_IThumbnailProvider;
	if (hkcr_ShellEx.isOpen()) {
		phkcr_IThumbnailProvider.reset(new RegKey(hkcr_ShellEx, IID_IThumbnailProvider_String, KEY_READ|KEY_WRITE, false));
		if (phkcr_IThumbnailProvider->isOpen()) {
			// Check if the default value matches the CLSID.
			const tstring str_IThumbnailProvider = phkcr_IThumbnailProvider->read(nullptr);
			if (str_IThumbnailProvider != CLSID_RP_ThumbnailProvider_String) {
				// Not our IThumbnailProvider.
				// We're done here.
				return ERROR_SUCCESS;
			}
		} else {
			// ERROR_FILE_NOT_FOUND is acceptable here.
			LONG lResult = phkcr_IThumbnailProvider->lOpenRes();
			if (lResult != ERROR_FILE_NOT_FOUND) {
				return lResult;
			}
			phkcr_IThumbnailProvider.reset(nullptr);
		}
	}

	// Restore the fallbacks if we have any.
	tstring clsid_reg;
	DWORD dwTypeTreatment = 0;
	DWORD treatment = 0;
	RegKey hkcr_RP_Fallback(hkey_Assoc, _T("RP_Fallback"), KEY_READ|KEY_WRITE, false);
	if (hkcr_RP_Fallback.isOpen()) {
		// Read the fallbacks.
		clsid_reg = hkcr_RP_Fallback.read(_T("IThumbnailProvider"));
		treatment = hkcr_RP_Fallback.read_dword(_T("Treatment"), &dwTypeTreatment);
	}

	if (phkcr_IThumbnailProvider) {
		if (!clsid_reg.empty()) {
			// Restore the IThumbnailProvider.
			LONG lResult = phkcr_IThumbnailProvider->write(nullptr, clsid_reg);
			if (lResult != ERROR_SUCCESS) {
				return lResult;
			}
			// Restore the "Treatment" value.
			if (dwTypeTreatment == REG_DWORD) {
				lResult = hkey_Assoc.write_dword(_T("Treatment"), treatment);
				if (lResult != ERROR_SUCCESS) {
					return lResult;
				}
			} else {
				// No "Treatment" value to restore.
				// Delete the current one if it's present.
				lResult = hkey_Assoc.deleteValue(_T("Treatment"));
				if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
					return lResult;
				}
			}
		} else {
			// No IThumbnailProvider to restore.
			// Remove the current one.
			phkcr_IThumbnailProvider->close();
			LONG lResult = hkcr_ShellEx.deleteSubKey(IID_IThumbnailProvider_String);
			if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
				return lResult;
			}

			// Remove the "Treatment" value if it's present.
			lResult = hkey_Assoc.deleteValue(_T("Treatment"));
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
		LONG lResult = hkcr_RP_Fallback.deleteValue(_T("IThumbnailProvider"));
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
		}
		lResult = hkcr_RP_Fallback.deleteValue(_T("Treatment"));
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
LONG RP_ThumbnailProvider::UnregisterFileType(_In_ RegKey &hkcr, _In_opt_ LPCTSTR ext)
{
	if (!ext) {
		// Unregister from hkcr directly.
		return RP_ThumbnailProvider_Private::UnregisterFileType(hkcr);
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
	LONG lResult = RP_ThumbnailProvider_Private::UnregisterFileType(hkcr_ext);
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
		lResult = RP_ThumbnailProvider_Private::UnregisterFileType(hkcr_ProgID);
	}

	// File type handler unregistered.
	return lResult;
}
