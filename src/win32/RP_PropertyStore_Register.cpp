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
	// TODO: InfoTip and PreviewTitle?
	static const wchar_t PreviewDetails[] = L"prop:"
		// Default properties. (Win7)
		// TODO: Reorder these. (Check .wav, .png, etc.)
		L"System.DateModified;"
		L"System.Size;"
		L"System.DateCreated;"
		// Custom properties.
		L"System.Title;"
		L"System.Company;"
		L"System.Image.Dimensions;"
		L"System.Media.Duration;"
		L"System.Media.SampleRate";
	// FIXME: Add standard system properties.
	static const wchar_t FullDetails[] = L"prop:"
		L"System.Title;"
		L"System.Company;"
		L"System.Image.Dimensions;"
		L"System.Image.Width;"
		L"System.Image.Height;"
		L"System.Media.Duration;"
		L"System.Media.SampleRate;";
		L"System.Media.SampleSize";

	RegKey hkey_ProgID(HKEY_CLASSES_ROOT, RP_ProgID, KEY_READ|KEY_WRITE, true);
	if (!hkey_ProgID.isOpen())
		return hkey_ProgID.lOpenRes();
	lResult = hkey_ProgID.write(L"PreviewDetails", PreviewDetails);
	if (lResult != ERROR_SUCCESS) return lResult;
	// FIXME: Need standard properties here.
	//lResult = hkey_ProgID.write(L"FullDetails", FullDetails);
	//if (lResult != ERROR_SUCCESS) return lResult;

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
