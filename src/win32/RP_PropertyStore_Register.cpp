/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_PropSheet_Register.cpp: IPropertyStore implementation.               *
 * COM registration functions.                                             *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RP_PropertyStore.hpp"
#include "RP_PropertyStore_p.hpp"

// libwin32common
using LibWin32UI::RegKey;

// C++ STL classes.
using std::tstring;
using std::unique_ptr;

#define CLSID_RP_PropertyStore_String	TEXT("{4A1E3510-50BD-4B03-A801-E4C954F43B96}")

/**
 * Get the PreviewDetails string.
 * @return PreviewDetails string.
 */
tstring RP_PropertyStore_Private::GetPreviewDetailsString()
{
	// PreviewDetails.
	// NOTE: Default properties should go *after* these.
	static const TCHAR PreviewDetails[] = _T("prop:")
		// Custom properties.
		_T("System.Title;")
		_T("System.Company;")
		_T("System.Author;")
		_T("System.FileDescription;")
		_T("System.Music.Composer;")
		_T("System.Media.Copyright;")
		_T("System.Image.Dimensions;")
		_T("System.Media.Duration;")
		_T("System.Media.SampleRate;");
		_T("com.gerbilsoft.rom-properties.GameID;");
	tstring s_previewDetails(PreviewDetails, _countof(PreviewDetails)-1);

	RegKey hkcr_All(HKEY_CLASSES_ROOT, _T("*"), KEY_READ, false);
	if (!hkcr_All.isOpen()) {
		// Unable to open "*".
		// Use the PreviewDetails as-is.
		return s_previewDetails;
	}

	// Get the default "PreviewDetails" and append them
	// to the custom "FullDetails".
	const tstring s_reg = hkcr_All.read(_T("PreviewDetails"));
	if (s_reg.size() > 5) {
		// First 5 characters should be "prop:".
		if (!_tcsnicmp(s_reg.c_str(), _T("prop:"), 5)) {
			// Append the properties.
			s_previewDetails += _T(';');
			s_previewDetails.append(s_reg.c_str()+5, s_reg.size()-5);
		}
	}

	return s_previewDetails;
}

/**
 * Get the InfoTip string.
 * @return InfoTip string.
 */
std::tstring RP_PropertyStore_Private::GetInfoTipString()
{
	// InfoTip.
	// NOTE: Default properties should go *before* these.
	static const TCHAR InfoTip[] =
		// Custom properties.
		_T("System.Title;")
		_T("System.Company;")
		_T("System.Author;")
		_T("System.FileDescription;")
		_T("System.Music.Composer;")
		_T("System.Media.Copyright;")
		_T("System.Image.Dimensions;")
		_T("System.Media.Duration;")
		_T("System.Media.SampleRate;")
		_T("com.gerbilsoft.rom-properties.GameID");

	RegKey hkcr_All(HKEY_CLASSES_ROOT, _T("*"), KEY_READ, false);
	if (!hkcr_All.isOpen()) {
		// Unable to open "*".
		// Use the InfoTip as-is.
		return tstring(InfoTip, _countof(InfoTip)-1);
	}

	// Get the default "InfoTip" and prepend them
	// to the custom "InfoTip".
	tstring s_infoTip;
	const tstring s_reg = hkcr_All.read(_T("InfoTip"));
	if (s_reg.size() > 5) {
		// First 5 characters should be "prop:".
		if (!_tcsnicmp(s_reg.c_str(), _T("prop:"), 5)) {
			// Prepend the properties.
			s_infoTip = s_reg;
		}
	}

	if (s_infoTip.empty()) {
		// Prepend with "prop:".
		s_infoTip = _T("prop:");
	} else {
		// Add a semicolon.
		s_infoTip += _T(';');
	}
	s_infoTip.append(InfoTip, _countof(InfoTip)-1);
	return s_infoTip;
}

/**
 * Get the FullDetails string.
 * @return FullDetails string.
 */
std::tstring RP_PropertyStore_Private::GetFullDetailsString()
{
	// FIXME: FullDetails will show empty properties if
	// they're listed here but aren't set by RP_PropertyStore.
	// We'll need to register multiple ProgIDs for different
	// classes of files, but maybe later...
	static const TCHAR FullDetails[] = _T("prop:")
		_T("System.PropGroup.General;")
		_T("System.Title;")
		_T("System.Company;")
		_T("System.FileDescription;")
		_T("System.PropGroup.Image;")
		_T("System.Image.Dimensions;")
		_T("System.Image.Width;")
		_T("System.Image.Height;")
		_T("System.PropGroup.Audio;")
		_T("System.Media.Duration;")
		_T("System.Audio.SampleRate;")
		_T("System.Audio.SampleSize");

	// TODO: Get the default FullDetails from the system.
	return tstring(FullDetails, _countof(FullDetails)-1);
}

/** Registration **/

/**
 * Register the file type handler.
 * @param hkcr	[in] HKEY_CLASSES_ROOT or user-specific classes root.
 * @param pHklm	[in,opt] HKEY_LOCAL_MACHINE or user-specific root, or nullptr to skip.
 * @param ext	[in] File extension, including the leading dot.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_PropertyStore::RegisterFileType(_In_ RegKey &hkcr, _In_opt_ RegKey *pHklm, _In_ LPCTSTR ext)
{
	// Set the properties to display in the various fields.
	// FIXME: FullDetails will show empty properties...
	// TODO: PreviewTitle.
	const tstring s_previewDetails = RP_PropertyStore_Private::GetPreviewDetailsString();
	const tstring s_infoTip = RP_PropertyStore_Private::GetInfoTipString();

	// TODO: verify this; make a separate function to register it.
	static int x = 0;
	if (!x) {
		x = 1;
		HRESULT hr = PSRegisterPropertySchema(L"C:\\rom-properties.propdesc");
		char buf[256];
		snprintf(buf, sizeof(buf), "PSRegisterPropertySchema HR: %08X", hr);
		MessageBoxA(NULL, buf, buf, 0);
	}

	// Write the registry keys.
	// TODO: Determine which fields are actually supported by the specific extension.
	// TODO: RP_Fallback handling?
	RegKey hkey_ext(hkcr, ext, KEY_READ|KEY_WRITE, true);
	if (!hkey_ext.isOpen())
		return hkey_ext.lOpenRes();
	LONG lResult = hkey_ext.write(_T("PreviewDetails"), s_previewDetails);
	if (lResult != ERROR_SUCCESS) return lResult;
	lResult = hkey_ext.write(_T("InfoTip"), s_infoTip);
	if (lResult != ERROR_SUCCESS) return lResult;
#if 0
	lResult = hkey_ext.write(_T("FullDetails"), s_fullDetails);
	if (lResult != ERROR_SUCCESS) return lResult;
#endif

	if (pHklm) {
		// Open the "PropertyHandlers" key.
		// NOTE: This key might not exist on ReactOS, so we'll need to create it.
		RegKey hklm_PropertyHandlers(*pHklm, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers"), KEY_READ, true);
		if (!hklm_PropertyHandlers.isOpen()) {
			return hklm_PropertyHandlers.lOpenRes();
		}

		// Open the file extension key.
		RegKey hklmph_ext(hklm_PropertyHandlers, ext, KEY_READ|KEY_WRITE, true);
		if (!hklmph_ext.isOpen()) {
			return hklmph_ext.lOpenRes();
		}
		hklm_PropertyHandlers.close();

		// Register our GUID as the property store handler.
		// TODO: Fallbacks?
		lResult = hklmph_ext.write(nullptr, CLSID_RP_PropertyStore_String);
		if (lResult != ERROR_SUCCESS) return lResult;
	}

	return ERROR_SUCCESS;
}

/**
 * Unregister the file type handler.
 * @param hkcr	[in] HKEY_CLASSES_ROOT or user-specific classes root.
 * @param pHklm	[in,opt] HKEY_LOCAL_MACHINE or user-specific root, or nullptr to skip.
 * @param ext	[in,opt] File extension, including the leading dot.
 *
 * NOTE: ext can be NULL, in which case, hkcr is assumed to be
 * the registered file association.
 *
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_PropertyStore::UnregisterFileType(_In_ RegKey &hkcr, _In_opt_ RegKey *pHklm, _In_opt_ LPCTSTR ext)
{
	// Check the main file extension key.
	// If PreviewDetails and InfoTip match our values, remove them.
	// FIXME: What if our version changes?
	// TODO: RP_Fallback handling?

	unique_ptr<RegKey> hkey_tmp;
	RegKey *pHkey;
	if (ext) {
		// ext is specified.
		hkey_tmp.reset(new RegKey(hkcr, ext, KEY_READ|KEY_WRITE, true));
		pHkey = hkey_tmp.get();
	} else {
		// No ext. Use hkcr directly.
		pHkey = &hkcr;
	}

	if (!pHkey->isOpen())
		return pHkey->lOpenRes();
	tstring s_value = pHkey->read(_T("PreviewDetails"));
	if (s_value == RP_PropertyStore_Private::GetPreviewDetailsString()) {
		LONG lResult = pHkey->deleteValue(_T("PreviewDetails"));
		if (lResult != ERROR_SUCCESS) return lResult;
	}
	s_value = pHkey->read(_T("InfoTip"));
	if (s_value == RP_PropertyStore_Private::GetInfoTipString()) {
		LONG lResult = pHkey->deleteValue(_T("InfoTip"));
		if (lResult != ERROR_SUCCESS) return lResult;
	}

	if (pHklm) {
		// Open the "PropertyHandlers" key.
		RegKey hklm_PropertyHandlers(*pHklm, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers"), KEY_READ, false);
		if (!hklm_PropertyHandlers.isOpen()) {
			return hklm_PropertyHandlers.lOpenRes();
		}

		// Open the file extension key.
		RegKey hklmph_ext(hklm_PropertyHandlers, ext, KEY_READ|KEY_WRITE, false);
		if (hklmph_ext.isOpen()) {
			// If our GUID is present as the property sheet handler,
			// remove it.
			const tstring def_value = hklmph_ext.read(nullptr);
			if (def_value == CLSID_RP_PropertyStore_String) {
				// Remove the default value.
				LONG lResult = hklmph_ext.deleteValue(nullptr);
				if (lResult != ERROR_SUCCESS) return lResult;
				// If there are no more values, delete the key.
				if (hklmph_ext.isKeyEmpty()) {
					hklmph_ext.close();
					// TODO: Return an error if this fails?
					hklm_PropertyHandlers.deleteSubKey(ext);
				}
			}
		} else {
			// Anything other than ERROR_FILE_NOT_FOUND is an error.
			LONG lResult = hklmph_ext.lOpenRes();
			if (lResult != ERROR_FILE_NOT_FOUND) {
				return lResult;
			}
		}
	}

	// We're done here.
	return ERROR_SUCCESS;
}
