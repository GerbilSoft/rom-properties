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
#include <cassert>
#include <cstring>

// C++ includes.
#include <string>
#include <list>
using std::wstring;
using std::list;

/**
 * Create or open a registry key.
 * @param hKeyRoot Root key.
 * @param path Path of the registry key.
 * @param samDesired Desired access rights.
 * @param create If true, create the key if it doesn't exist.
 */
RegKey::RegKey(HKEY hKeyRoot, LPCWSTR path, REGSAM samDesired, bool create)
	: m_hKey(nullptr)
	, m_lOpenRes(ERROR_SUCCESS)
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
	: m_hKey(nullptr)
	, m_lOpenRes(ERROR_SUCCESS)
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
 * Read a string value from a key. (REG_SZ, REG_EXPAND_SZ)
 * NOTE: REG_EXPAND_SZ values are NOT expanded.
 * @param lpValueName	[in] Value name. (Use nullptr or an empty string for the default value.)
 * @param lpType	[out,opt] Variable to store the key type in. (REG_NONE, REG_SZ, or REG_EXPAND_SZ)
 * @return String value, or empty string on error. (check lpType)
 */
wstring RegKey::read(LPCWSTR lpValueName, LPDWORD lpType) const
{
	if (!m_hKey) {
		// Handle is invalid.
		if (lpType) {
			*lpType = REG_NONE;
		}
		return wstring();
	}

	// Determine the required buffer size.
	DWORD cbData, dwType;
	LONG lResult = RegQueryValueEx(m_hKey,
		lpValueName,	// lpValueName
		nullptr,	// lpReserved
		&dwType,	// lpType
		nullptr,	// lpData
		&cbData);	// lpcbData
	if (lResult != ERROR_SUCCESS || cbData == 0 ||
	    (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
	{
		// Either an error occurred, or this isn't REG_SZ or REG_EXPAND_SZ.
		if (lpType) {
			*lpType = REG_NONE;
		}
		return wstring();
	}

	// Allocate a buffer and get the data.
	wchar_t *wbuf = static_cast<wchar_t*>(malloc(cbData));
	if (!wbuf) {
		// Memory allocation failed.
		return wstring();
	}
	lResult = RegQueryValueEx(m_hKey,
		lpValueName,	// lpValueName
		nullptr,	// lpReserved
		&dwType,	// lpType
		(LPBYTE)wbuf,	// lpData
		&cbData);	// lpcbData
	if (lResult != ERROR_SUCCESS ||
	    (dwType != REG_SZ && dwType != REG_EXPAND_SZ)) {
		// Either an error occurred, or this isn't REG_SZ or REG_EXPAND_SZ.
		free(wbuf);
		if (lpType) {
			*lpType = REG_NONE;
		}
		return wstring();
	}

	// Save the key type.
	if (lpType) {
		*lpType = dwType;
	}

	// Convert cbData to cchData.
	assert(cbData % 2 == 0);
	DWORD cchData = cbData / sizeof(wbuf[0]);

	// Check for NULL terminators.
	for (; cchData > 0; cchData--) {
		if (wbuf[cchData-1] != 0)
			break;
	}

	if (cchData == 0) {
		// No actual string data.
		free(wbuf);
		return wstring();
	}

	// Return the string.
	wstring wstr(wbuf, cchData);
	free(wbuf);
	return wstr;
}

/**
 * Read a DWORD value from a key. (REG_DWORD)
 * @param lpValueName	[in] Value name. (Use nullptr or an empty string for the default value.)
 * @param lpType	[out,opt] Variable to store the key type in. (REG_NONE or REG_DWORD)
 * @return DWORD value, or 0 on error. (check lpType)
 */
DWORD RegKey::read_dword(LPCWSTR lpValueName, LPDWORD lpType) const
{
	if (!m_hKey) {
		// Handle is invalid.
		if (lpType) {
			*lpType = REG_NONE;
		}
		return 0;
	}

	// Get the DWORD.
	DWORD data, cbData, dwType;
	cbData = sizeof(data);
	LONG lResult = RegQueryValueEx(m_hKey,
		lpValueName,	// lpValueName
		nullptr,	// lpReserved
		&dwType,	// lpType
		(LPBYTE)&data,	// lpData
		&cbData);	// lpcbData
	if (lResult != ERROR_SUCCESS || cbData == 0 || dwType != REG_DWORD) {
		// Either an error occurred, or this isn't REG_DWORD.
		if (lpType) {
			*lpType = REG_NONE;
		}
		return 0;
	}

	// Return the DWORD.
	if (lpType) {
		*lpType = dwType;
	}
	return data;
}

/**
 * Write a string value to this key.
 * @param lpValueName Value name. (Use nullptr or an empty string for the default value.)
 * @param value Value.
 * @param dwType Key type. (REG_SZ or REG_EXPAND_SZ)
 * @return RegSetValueEx() return value.
 */
LONG RegKey::write(LPCWSTR lpValueName, LPCWSTR value, DWORD dwType)
{
	assert(m_hKey != nullptr);
	assert(dwType == REG_SZ || dwType == REG_EXPAND_SZ);
	if (!m_hKey) {
		// Handle is invalid.
		return ERROR_INVALID_HANDLE;
	} else if (dwType != REG_SZ && dwType != REG_EXPAND_SZ) {
		// Invalid type.
		return ERROR_INVALID_PARAMETER;
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

	return RegSetValueEx(m_hKey,
		lpValueName,		// lpValueName
		0,			// Reserved
		REG_SZ,			// dwType
		(const BYTE*)value,	// lpData
		cbData);		// cbData
}

/**
 * Write a string value to this key.
 * @param lpValueName Value name. (Use nullptr or an empty string for the default value.)
 * @param value Value.
 * @param dwType Key type. (REG_SZ or REG_EXPAND_SZ)
 * @return RegSetValueEx() return value.
 */
LONG RegKey::write(LPCWSTR lpValueName, const wstring& value, DWORD dwType)
{
	assert(m_hKey != nullptr);
	assert(dwType == REG_SZ || dwType == REG_EXPAND_SZ);
	if (!m_hKey) {
		// Handle is invalid.
		return ERROR_INVALID_HANDLE;
	} else if (dwType != REG_SZ && dwType != REG_EXPAND_SZ) {
		// Invalid type.
		return ERROR_INVALID_PARAMETER;
	}

	// Get the string length, add 1 for NULL,
	// and multiply by sizeof(wchar_t).
	DWORD cbData = (DWORD)((value.size() + 1) * sizeof(wchar_t));

	return RegSetValueEx(m_hKey,
		lpValueName,			// lpValueName
		0,				// Reserved
		dwType,				// dwType
		(const BYTE*)value.c_str(),	// lpData
		cbData);			// cbData
}

/**
 * Write a DWORD value to this key.
 * @param lpValueName Value name. (Use nullptr or an empty string for the default value.)
 * @param value Value.
 * @return RegSetValueEx() return value.
 */
LONG RegKey::write_dword(LPCWSTR lpValueName, DWORD value)
{
	if (!m_hKey) {
		// Handle is invalid.
		return ERROR_INVALID_HANDLE;
	}

	// Get the string length, add 1 for NULL,
	// and multiply by sizeof(wchar_t).
	DWORD cbData = (DWORD)sizeof(value);

	return RegSetValueEx(m_hKey,
		lpValueName,		// lpValueName
		0,			// Reserved
		REG_DWORD,		// dwType
		(const BYTE*)&value,	// lpData
		cbData);		// cbData
}

/**
 * Delete a value.
 * @param lpValueName Value name. (Use nullptr or an empty string for the default value.)
 * @return RegDeleteValue() return value.
 */
LONG RegKey::deleteValue(LPCWSTR lpValueName)
{
	if (!m_hKey) {
		// Handle is invalid.
		return ERROR_INVALID_HANDLE;
	}

	// Delete the value.
	return RegDeleteValue(m_hKey, lpValueName);
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

	// Get the maximum subkey name length.
	DWORD cMaxSubKeyLen;
	lResult = RegQueryInfoKey(hSubKey,
		nullptr, nullptr,		// lpClass, lpcClass
		nullptr,			// lpReserved
		nullptr, &cMaxSubKeyLen,	// lpcSubKeys, lpcMaxSubKeyLen
		nullptr, nullptr,		// lpcMaxClassLen, lpcValues
		nullptr, nullptr,		// lpcMaxValueNameLen, lpcMaxValueLen
		nullptr, nullptr)	;	// lpcbSecurityDescriptor, lpftLastWriteTime
	if (lResult != ERROR_SUCCESS) {
		RegCloseKey(hSubKey);
		return lResult;
	}

	// cMaxSubKeyLen doesn't include the NULL terminator, so add one.
	++cMaxSubKeyLen;
	wchar_t *szName = static_cast<wchar_t*>(malloc(cMaxSubKeyLen * sizeof(wchar_t)));

	// Enumerate the keys.
	DWORD dwSize = cMaxSubKeyLen;
	lResult = RegEnumKeyEx(hSubKey,
		0,		// dwIndex
		szName,		// lpName
		&dwSize,	// lpcName
		nullptr,	// lpReserved
		nullptr,	// lpClass
		nullptr,	// lpcClass
		nullptr);	// lpftLastWriteTime
	if (lResult == ERROR_SUCCESS) {
		do {
			// Recurse through this subkey.
			lResult = deleteSubKey(hSubKey, szName);
			if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
				break;

			// Next subkey.
			dwSize = cMaxSubKeyLen;
			lResult = RegEnumKeyEx(hSubKey,
				0,		// dwIndex
				szName,		// lpName
				&dwSize,	// lpcName
				nullptr,	// lpReserved
				nullptr,	// lpClass
				nullptr,	// lpcClass
				nullptr);	// lpftLastWriteTime
		} while (lResult == ERROR_SUCCESS);
	}
	free(szName);
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

/**
 * Enumerate subkeys.
 * @param lstSubKeys List to place the subkey names in.
 * @return ERROR_SUCCESS on success; WinAPI error on error.
 */
LONG RegKey::enumSubKeys(list<wstring> &lstSubKeys)
{
	if (!m_hKey) {
		// Handle is invalid.
		return ERROR_INVALID_HANDLE;
	}

	LONG lResult;
	DWORD cSubKeys, cchMaxSubKeyLen;

	// Get the number of subkeys.
	lResult = RegQueryInfoKey(m_hKey,
		nullptr, nullptr,	// lpClass, lpcClass
		nullptr,		// lpReserved
		&cSubKeys, &cchMaxSubKeyLen,
		nullptr, nullptr,	// lpcMaxClassLen, lpcValues
		nullptr, nullptr,	// lpcMaxValueNameLen, lpcMaxValueLen
		nullptr, nullptr);	// lpcbSecurityDescriptor, lpftLastWriteTime
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// cchMaxSubKeyLen doesn't include the NULL terminator.
	cchMaxSubKeyLen++;

	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms724872(v=vs.85).aspx says
	// key names are limited to 255 characters, but who knows...
	wchar_t *wbuf = static_cast<wchar_t*>(malloc(cchMaxSubKeyLen * sizeof(wchar_t)));

	// Initialize the vector.
	lstSubKeys.clear();

	for (int i = 0; i < (int)cSubKeys; i++) {
		DWORD cchName = cchMaxSubKeyLen;
		lResult = RegEnumKeyEx(m_hKey, i,
			wbuf, &cchName,
			nullptr,	// lpReserved
			nullptr,	// lpClass
			nullptr,	// lpcClass
			nullptr);	// lpftLastWriteTime
		if (lResult != ERROR_SUCCESS) {
			free(wbuf);
			return lResult;
		}

		// Add the subkey name to the return vector.
		// cchName contains the number of characters in the
		// subkey name, NOT including the NULL terminator.
		lstSubKeys.push_back(wstring(wbuf, cchName));
	}

	free(wbuf);
	return ERROR_SUCCESS;
}

/**
 * Is the key empty?
 * This means no values, an empty default value, and no subkey.
 * @return True if the key is empty; false if not or if an error occurred.
 */
bool RegKey::isKeyEmpty(void)
{
	if (!m_hKey) {
		// Handle is invalid.
		// TODO: Better error reporting.
		return false;
	}

	LONG lResult;
	DWORD cSubKeys, cValues;

	// Get the number of subkeys.
	lResult = RegQueryInfoKey(m_hKey,
		nullptr, nullptr,	// lpClass, lpcClass
		nullptr,		// lpReserved
		&cSubKeys, nullptr,	// lpcSubKeys, lpcMaxSubKeyLen
		nullptr, &cValues,	// lpcMaxClassLen, lpcValues
		nullptr, nullptr,	// lpcMaxValueNameLen, lpcMaxValueLen
		nullptr, nullptr);	// lpcbSecurityDescriptor, lpftLastWriteTime
	if (lResult != ERROR_SUCCESS) {
		// TODO: Better error reporting.
		return false;
	}

	if (cSubKeys > 0 || cValues > 0) {
		// We have at least one subkey or value.
		// NOTE: The default value is included in cValues,
		// so we don't have to check for it separately.
		return false;
	}

	// Key is empty.
	return true;
}

/** COM registration convenience functions. **/

/**
 * Register a file type.
 * @param fileType File extension, with leading dot. (e.g. ".bin")
 * @param pHkey_Assoc Pointer to RegKey* to store opened registry key on success. (If nullptr, key will be closed.)
 * @return ERROR_SUCCESS on success; WinAPI error on error.
 */
LONG RegKey::RegisterFileType(LPCWSTR fileType, RegKey **pHkey_Assoc)
{
	// TODO: Handle cases where the user has already selected
	// a file association?

	// Create/open the file type key.
	// If we're returning the key, we need read/write access;
	// otherwise, we only need write access.
	const REGSAM samDesired = (pHkey_Assoc ? KEY_READ | KEY_WRITE : KEY_WRITE);

	RegKey *pHkcr_fileType = new RegKey(HKEY_CLASSES_ROOT, fileType, samDesired, true);
	if (!pHkcr_fileType->isOpen()) {
		// Error opening the key.
		LONG lResult = pHkcr_fileType->lOpenRes();
		delete pHkcr_fileType;
		return lResult;
	}

	if (pHkey_Assoc) {
		// Return the RegKey.
		*pHkey_Assoc = pHkcr_fileType;
	} else {
		// We're done using the RegKey.
		delete pHkcr_fileType;
	}

	return ERROR_SUCCESS;
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

#ifndef NDEBUG
	// Debug build: Disable process isolation to make debugging easier.
	lResult = hkcr_Obj_CLSID.write_dword(L"DisableProcessIsolation", 1);
	if (lResult != ERROR_SUCCESS)
		return lResult;
#else
	// Release build: Enable process isolation for increased robustness.
	lResult = hkcr_Obj_CLSID.deleteValue(L"DisableProcessIsolation");
	if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
		return lResult;
#endif

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
	// NOTE: deleteSubKey() may return ERROR_FILE_NOT_FOUND,
	// which indicates the object was already unregistered.
	lResult = hkcr_CLSID.deleteSubKey(clsid_str);
	if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
		return lResult;

	// Open the approved shell extensions key.
	RegKey hklm_Approved(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved",
		KEY_WRITE, false);
	if (!hklm_Approved.isOpen())
		return hklm_Approved.lOpenRes();

	// Remove the value for the specified CLSID.
	lResult = hklm_Approved.deleteValue(clsid_str);
	if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
		return lResult;

	// TODO: Check progID and remove CLSID if it's present.
	return ERROR_SUCCESS;
}
