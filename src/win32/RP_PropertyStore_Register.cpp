/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_PropSheet_Register.cpp: IPropertyStore implementation.               *
 * COM registration functions.                                             *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "stdafx.h"
#include "RP_PropertyStore.hpp"
#include "RP_PropertyStore_p.hpp"

#include "libwin32common/RegKey.hpp"
using LibWin32Common::RegKey;

// C++ includes.
#include <string>
using std::wstring;

#define CLSID_RP_PropertyStore_String	TEXT("{4A1E3510-50BD-4B03-A801-E4C954F43B96}")

/**
 * Register the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_PropertyStore::RegisterCLSID(void)
{
	static const wchar_t description[] = L"ROM Properties Page - Property Store";
	extern const wchar_t RP_ProgID[];

	// Register the COM object.
	LONG lResult = RegKey::RegisterComObject(__uuidof(RP_PropertyStore), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_PropertyStore), description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Set the properties to display in the various fields.
	// TODO: Get properties from RomDataFactory, or just use
	// all of them regardless?
	// TODO: PreviewTitle.

	// PreviewDetails.
	// NOTE: Default properties should go *after* these.
	static const wchar_t PreviewDetails[] = L"prop:"
		// Custom properties.
		L"System.Title;"
		L"System.Company;"
		L"System.Music.Composer;"
		L"System.Media.Copyright;"
		L"System.Image.Dimensions;"
		L"System.Media.Duration;"
		L"System.Media.SampleRate;";

	// InfoTip.
	// NOTE: Default properties should go *before* these.
	static const wchar_t InfoTip[] =
		// Custom properties.
		L"System.Title;"
		L"System.Company;"
		L"System.Music.Composer;"
		L"System.Media.Copyright;"
		L"System.Image.Dimensions;"
		L"System.Media.Duration;"
		L"System.Media.SampleRate";

#if 0
	// FIXME: FullDetails will show empty properties if
	// they're listed here but aren't set by RP_PropertyStore.
	// We'll need to register multiple ProgIDs for different
	// classes of files, but maybe later...
	static const wchar_t FullDetails[] = L"prop:"
		L"System.PropGroup.General;"
		L"System.Title;"
		L"System.Company;"
		L"System.PropGroup.Image;"
		L"System.Image.Dimensions;"
		L"System.Image.Width;"
		L"System.Image.Height;"
		L"System.PropGroup.Audio;"
		L"System.Media.Duration;"
		L"System.Audio.SampleRate;"
		L"System.Audio.SampleSize";
#endif

	RegKey hkcr_All(HKEY_CLASSES_ROOT, L"*", KEY_READ, false);
	if (!hkcr_All.isOpen())
		return hkcr_All.lOpenRes();
	wstring s_reg;

	// Get the default "PreviewDetails" and append them
	// to the custom "FullDetails".
	wstring s_previewDetails(PreviewDetails, ARRAY_SIZE(PreviewDetails)-1);
	s_reg = hkcr_All.read(L"PreviewDetails");
	if (s_reg.size() > 5) {
		// First 5 characters should be "prop:".
		if (!wcsncasecmp(s_reg.c_str(), L"prop:", 5)) {
			// Append the properties.
			s_previewDetails += L';';
			s_previewDetails.append(s_reg.c_str()+5, s_reg.size()-5);
		}
	}

	// Get the default "InfoTip" and prepend them
	// to the custom "InfoTip".
	wstring s_infoTip;
	s_reg = hkcr_All.read(L"PreviewDetails");
	if (s_reg.size() > 5) {
		// First 5 characters should be "prop:".
		if (!wcsncasecmp(s_reg.c_str(), L"prop:", 5)) {
			// Prepend the properties.
			s_infoTip = s_reg;
		}
	}
	if (s_infoTip.empty()) {
		// Prepend with "prop:".
		s_infoTip = L"prop:";
	}
	s_infoTip.append(InfoTip, ARRAY_SIZE(InfoTip)-1);

	// Write the registry keys.
	RegKey hkey_ProgID(HKEY_CLASSES_ROOT, RP_ProgID, KEY_READ|KEY_WRITE, true);
	if (!hkey_ProgID.isOpen())
		return hkey_ProgID.lOpenRes();
	lResult = hkey_ProgID.write(L"PreviewDetails", s_previewDetails);
	if (lResult != ERROR_SUCCESS) return lResult;
	lResult = hkey_ProgID.write(L"InfoTip", s_infoTip);
	if (lResult != ERROR_SUCCESS) return lResult;
#if 0
	lResult = hkey_ProgID.write(L"FullDetails", s_fullDetails);
	if (lResult != ERROR_SUCCESS) return lResult;
#endif

	// COM object registered.
	return ERROR_SUCCESS;
}

/**
 * Register the file type handler.
 * @param hklm HKEY_LOCAL_MACHINE or user-specific root.
 * @param ext File extension, including the leading dot.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_PropertyStore::RegisterFileType(RegKey &hklm, LPCWSTR ext)
{
	// Open the "PropertyHandlers" key.
	RegKey hklm_PropertyHandlers(hklm, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers", KEY_READ, false);
	if (!hklm_PropertyHandlers.isOpen()) {
		return hklm_PropertyHandlers.lOpenRes();
	}

	// Open the file extension key.
	RegKey hklmph_ext(hklm_PropertyHandlers, ext, KEY_READ|KEY_WRITE, true);
	if (!hklmph_ext.isOpen()) {
		return hklmph_ext.lOpenRes();
	}
	hklm_PropertyHandlers.close();

	// Register our GUID as the property sheet handler.
	// TODO: Fallbacks?
	return hklmph_ext.write(nullptr, CLSID_RP_PropertyStore_String);
}

/**
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_PropertyStore::UnregisterCLSID(void)
{
	extern const wchar_t RP_ProgID[];

	// Unegister the COM object.
	LONG lResult = RegKey::UnregisterComObject(__uuidof(RP_PropertyStore), RP_ProgID);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Remove the ProgID property store values.
	RegKey hkey_ProgID(HKEY_CLASSES_ROOT, RP_ProgID, KEY_READ|KEY_WRITE, false);
	if (hkey_ProgID.isOpen()) {
		hkey_ProgID.deleteValue(L"InfoTip");
		hkey_ProgID.deleteValue(L"FullDetails");
		hkey_ProgID.deleteValue(L"PreviewDetails");
		hkey_ProgID.deleteValue(L"PreviewTitle");
	}

	return ERROR_SUCCESS;
}

/**
 * Unregister the file type handler.
 * @param hklm HKEY_LOCAL_MACHINE or user-specific root.
 * @param ext File extension, including the leading dot.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_PropertyStore::UnregisterFileType(RegKey &hklm, LPCWSTR ext)
{
	// Open the "PropertyHandlers" key.
	RegKey hklm_PropertyHandlers(hklm, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers", KEY_READ, false);
	if (!hklm_PropertyHandlers.isOpen()) {
		return hklm_PropertyHandlers.lOpenRes();
	}

	// Open the file extension key.
	RegKey hklmph_ext(hklm_PropertyHandlers, ext, KEY_READ|KEY_WRITE, false);
	if (!hklmph_ext.isOpen()) {
		// Anything other than ERROR_FILE_NOT_FOUND is an error.
		LONG lResult = hklmph_ext.lOpenRes();
		if (lResult == ERROR_FILE_NOT_FOUND)
			lResult = ERROR_SUCCESS;
		return lResult;
	}

	// If our GUID is present as the property sheet handler,
	// remove it.
	wstring def_value = hklmph_ext.read(nullptr);
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

	// We're done here.
	return ERROR_SUCCESS;
}
