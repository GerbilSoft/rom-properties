/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RegKey.hpp: Registry key wrapper.                                       *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
#include "RegKey.hpp"

// C includes. (C++ namespace)
#include <cstring>

/**
 * Create or open a registry key.
 * @param hKeyRoot Root key.
 * @param path Path of the registry key.
 * @param samDesired Desired access rights.
 * @param create If true, create the key if it doesn't exist.
 */
RegKey::RegKey(HKEY hKeyRoot, LPCWSTR path, REGSAM samDesired, bool create)
	: m_lOpenRes(ERROR_SUCCESS)
	, m_hKey(nullptr)
	, m_samDesired(samDesired)
{
	if (create) {
		// REG_CREATE
		m_lOpenRes = RegCreateKeyEx(hKeyRoot, path, 0, nullptr, 0,
			samDesired, nullptr, &m_hKey, nullptr);
	} else {
		// REG_OPEN
		m_lOpenRes = RegOpenKeyEx(hKeyRoot, path, 0, samDesired, &m_hKey);
	}

	if (m_lOpenRes != ERROR_SUCCESS) {
		// Error creating or opening the key.
		m_hKey = nullptr;
	}
}

/**
 * Create or open a registry key.
 * @param root Root key.
 * @param path Path of the registry key.
 * @param samDesired Desired access rights.
 * @param create If true, create the key if it doesn't exist.
 */
RegKey::RegKey(const RegKey& root, LPCWSTR path, REGSAM samDesired, bool create)
	: m_lOpenRes(ERROR_SUCCESS)
	, m_hKey(nullptr)
	, m_samDesired(samDesired)
{
	if (create) {
		// REG_CREATE
		m_lOpenRes = RegCreateKeyEx(root.handle(), path, 0, nullptr, 0,
			samDesired, nullptr, &m_hKey, nullptr);
	} else {
		// REG_OPEN
		m_lOpenRes = RegOpenKeyEx(root.handle(), path, 0, samDesired, &m_hKey);
	}

	if (m_lOpenRes != ERROR_SUCCESS) {
		// Error creating or opening the key.
		m_hKey = nullptr;
	}
}

RegKey::~RegKey()
{
	if (m_hKey != nullptr) {
		RegCloseKey(m_hKey);
	}
}

/**
 * Get the handle to the opened registry key.
 * @return Handle to the opened registry key, or INVALID_HANDLE_VALUE if not open.
 */
HKEY RegKey::handle(void) const
{
	return m_hKey;
}

/**
 * Was the key opened successfully?
 * @return True if the key was opened successfully; false if not.
 */
bool RegKey::isOpen(void) const
{
	return (m_hKey != nullptr);
}

/**
 * Get the return value of RegCreateKeyEx() or RegOpenKeyEx().
 * @return Return value.
 */
LONG RegKey::lOpenRes(void) const
{
	return m_lOpenRes;
}

/**
 * Get the key's desired access rights.
 * @return Desired access rights.
 */
REGSAM RegKey::samDesired(void) const
{
	return m_samDesired;
}

/**
 * Close the key.
 */
void RegKey::close(void)
{
	if (m_hKey != nullptr) {
		RegCloseKey(m_hKey);
		m_hKey = nullptr;
	}
}

/** Basic registry access functions. **/

/**
 * Write a value to this key.
 * @param lpValueName Value name. (Use nullptr or an empty string for the default value.)
 * @param value Value.
 * @return RegSetValueEx() return value.
 */
LONG RegKey::write(LPCWSTR lpValueName, LPCWSTR value)
{
	if (!m_hKey) {
		// Handle is invalid.
		return ERROR_INVALID_HANDLE;
	}

	DWORD cbData;
	if (!value) {
		// No string.
		cbData = 0;
	} else {
		// Get the string length, add 1 for NULL,
		// and multiply by sizeof(wchar_t).
		cbData = (DWORD)((wcslen(value) + 1) * sizeof(wchar_t));
	}

	return RegSetValueEx(m_hKey, lpValueName, 0, REG_SZ,
		reinterpret_cast<const BYTE*>(value), cbData);
}

/**
 * Write a value to this key.
 * @param lpValueName Value name. (Use nullptr or an empty string for the default value.)
 * @param value Value.
 * @return RegSetValueEx() return value.
 */
LONG RegKey::write(LPCWSTR lpValueName, const std::wstring& value)
{
	if (!m_hKey) {
		// Handle is invalid.
		return ERROR_INVALID_HANDLE;
	}

	// Get the string length, add 1 for NULL,
	// and multiply by sizeof(wchar_t).
	DWORD cbData = (DWORD)((value.size() + 1) * sizeof(wchar_t));

	return RegSetValueEx(m_hKey, lpValueName, 0, REG_SZ,
		reinterpret_cast<const BYTE*>(value.c_str()), cbData);
}

/**
 * Recursively delete a subkey.
 * @param hKeyRoot Root key.
 * @param lpSubKey Subkey name.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RegKey::deleteSubKey(HKEY hKeyRoot, LPCWSTR lpSubKey)
{
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms724235(v=vs.85).aspx
	if (!hKeyRoot || !lpSubKey || !lpSubKey[0]) {
		// nullptr specified, or lpSubKey is empty.
		return ERROR_INVALID_PARAMETER;
	}

	// Attempt to delete the key directly without recursing.
	LONG lResult = RegDeleteKey(hKeyRoot, lpSubKey);
	if (lResult == ERROR_SUCCESS) {
		// Key deleted. We're done here.
		return ERROR_SUCCESS;
	}

	// We need to recurse into the key and delete all subkeys.
	HKEY hSubKey;
	lResult = RegOpenKeyEx(hKeyRoot, lpSubKey, 0, KEY_READ, &hSubKey);
	if (lResult != ERROR_SUCCESS) {
		// Something failed.
		return lResult;
	}

	// Enumerate the keys.
	wchar_t szName[MAX_PATH];
	DWORD dwSize = (DWORD)(sizeof(szName)/sizeof(szName[0]));
	lResult = RegEnumKeyEx(hSubKey, 0, szName, &dwSize, nullptr, nullptr, nullptr, nullptr);
	if (lResult == ERROR_SUCCESS) {
		do {
			// Recurse through this subkey.
			lResult = deleteSubKey(hSubKey, szName);
			if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
				break;

			dwSize = MAX_PATH;
			lResult = RegEnumKeyEx(hSubKey, 0, szName, &dwSize, nullptr, nullptr, nullptr, nullptr);
		} while (lResult == ERROR_SUCCESS);
	}

	RegCloseKey(hSubKey);

	// Try to delete the key again.
	return RegDeleteKey(hKeyRoot, lpSubKey);
}

/**
 * Recursively delete a subkey.
 * @param lpSubKey Subkey name.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RegKey::deleteSubKey(LPCWSTR lpSubKey)
{
	if (!m_hKey) {
		// Handle is invalid.
		return ERROR_INVALID_HANDLE;
	}

	return deleteSubKey(m_hKey, lpSubKey);
}

/** COM registration convenience functions. **/

/**
 * Register a file type.
 * @param fileType File extension, with leading dot. (e.g. ".bin")
 * @param progID ProgID.
 * @return ERROR_SUCCESS on success; WinAPI error on error.
 */
LONG RegKey::RegisterFileType(LPCWSTR fileType, LPCWSTR progID)
{
	// TODO: Handle cases where the user has already selected
	// a file association?

	// Create/open the file type key.
	RegKey hkcr_fileType(HKEY_CLASSES_ROOT, fileType, KEY_WRITE, true);
	if (!hkcr_fileType.isOpen())
		return hkcr_fileType.lOpenRes();

	// Set the default value to the ProgID.
	return hkcr_fileType.write(nullptr, progID);
}

/**
 * Register a COM object in this DLL.
 * @param rclsid CLSID.
 * @param progID ProgID.
 * @param description Description of the COM object.
 * @return ERROR_SUCCESS on success; WinAPI error on error.
 */
LONG RegKey::RegisterComObject(REFCLSID rclsid, LPCWSTR progID, LPCWSTR description)
{
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(rclsid, clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0)
		return ERROR_INVALID_PARAMETER;

	// Open HKCR\CLSID.
	RegKey hkcr_CLSID(HKEY_CLASSES_ROOT, L"CLSID", KEY_WRITE, false);
	if (!hkcr_CLSID.isOpen())
		return hkcr_CLSID.lOpenRes();

	// Create a key using the CLSID.
	RegKey hkcr_Obj_CLSID(hkcr_CLSID, clsid_str, KEY_WRITE, true);
	if (!hkcr_Obj_CLSID.isOpen())
		return hkcr_Obj_CLSID.lOpenRes();
	// Set the default value to the descirption of the COM object.
	lResult = hkcr_Obj_CLSID.write(nullptr, description);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// Create an InprocServer32 subkey.
	RegKey hkcr_InprocServer32(hkcr_Obj_CLSID, L"InprocServer32", KEY_WRITE, true);
	if (!hkcr_InprocServer32.isOpen())
		return hkcr_InprocServer32.lOpenRes();
	// Set the default value to the DLL filename.
	extern wchar_t dll_filename[MAX_PATH];
	if (dll_filename[0] != 0) {
		lResult = hkcr_InprocServer32.write(nullptr, dll_filename);
		if (lResult != ERROR_SUCCESS)
			return lResult;
	}

	// Set the threading model to Apartment.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/cc144110%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
	lResult = hkcr_InprocServer32.write(L"ThreadingModel", L"Apartment");
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// Create a ProgID subkey.
	RegKey hkcr_Obj_CLSID_ProgID(hkcr_Obj_CLSID, L"ProgID", KEY_WRITE, true);
	if (!hkcr_Obj_CLSID_ProgID.isOpen())
		return hkcr_Obj_CLSID_ProgID.lOpenRes();
	// Set the default value.
	return hkcr_Obj_CLSID_ProgID.write(nullptr, progID);
}

/**
 * Register a shell extension as an approved extension.
 * @param rclsid CLSID.
 * @param description Description of the shell extension.
 * @return ERROR_SUCCESS on success; WinAPI error on error.
 */
LONG RegKey::RegisterApprovedExtension(REFCLSID rclsid, LPCWSTR description)
{
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(rclsid, clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0)
		return ERROR_INVALID_PARAMETER;

	// Open the approved shell extensions key.
	RegKey hklm_Approved(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved",
		KEY_WRITE, false);
	if (!hklm_Approved.isOpen())
		return hklm_Approved.lOpenRes();

	// Create a value for the specified CLSID.
	return hklm_Approved.write(clsid_str, description);
}

/**
 * Unregister a COM object in this DLL.
 * @param rclsid CLSID.
 * @param progID ProgID.
 * @return ERROR_SUCCESS on success; WinAPI error on error.
 */
LONG RegKey::UnregisterComObject(REFCLSID rclsid, LPCWSTR progID)
{
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(rclsid, clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0)
		return ERROR_INVALID_PARAMETER;

	// Open HKCR\CLSID.
	RegKey hkcr_CLSID(HKEY_CLASSES_ROOT, L"CLSID", KEY_WRITE, false);
	if (!hkcr_CLSID.isOpen())
		return hkcr_CLSID.lOpenRes();

	// Delete the CLSID key.
	lResult = hkcr_CLSID.deleteSubKey(clsid_str);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// TODO: Check progID and remove CLSID if it's present.
	return ERROR_SUCCESS;
}
