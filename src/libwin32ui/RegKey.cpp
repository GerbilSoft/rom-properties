/***************************************************************************
 * ROM Properties Page shell extension. (libwin32ui)                       *
 * RegKey.hpp: Registry key wrapper.                                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RegKey.hpp"

// C includes (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes
#include <memory>
using std::forward_list;
using std::unique_ptr;
using std::tstring;

// Windows SDK
#include <objbase.h>

namespace LibWin32UI {

/**
 * Create or open a registry key.
 * @param hKeyRoot Root key.
 * @param path Path of the registry key.
 * @param samDesired Desired access rights.
 * @param create If true, create the key if it doesn't exist.
 */
RegKey::RegKey(HKEY hKeyRoot, LPCTSTR path, REGSAM samDesired, bool create)
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
RegKey::RegKey(const RegKey& root, LPCTSTR path, REGSAM samDesired, bool create)
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
tstring RegKey::read(LPCTSTR lpValueName, LPDWORD lpType) const
{
	if (!m_hKey) {
		// Handle is invalid.
		if (lpType) {
			*lpType = REG_NONE;
		}
		return {};
	}

	// Determine the required buffer size.
	DWORD cbData, dwType;
	LONG lResult = RegQueryValueEx(m_hKey,
		lpValueName,	// lpValueName
		nullptr,	// lpReserved
		&dwType,	// lpType
		nullptr,	// lpData
		&cbData);	// lpcbData
	if (lResult != ERROR_SUCCESS || cbData == 0 || (cbData % sizeof(TCHAR) != 0) ||
	    (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
	{
		// Either an error occurred, or this isn't REG_SZ or REG_EXPAND_SZ.
		if (lpType) {
			*lpType = REG_NONE;
		}
		return {};
	}

	// Allocate a buffer and get the data.
	unique_ptr<TCHAR[]> tbuf(new TCHAR[cbData/sizeof(TCHAR)]);
	lResult = RegQueryValueEx(m_hKey,
		lpValueName,		// lpValueName
		nullptr,		// lpReserved
		&dwType,		// lpType
		(LPBYTE)tbuf.get(),	// lpData
		&cbData);		// lpcbData
	if (lResult != ERROR_SUCCESS ||
	    (dwType != REG_SZ && dwType != REG_EXPAND_SZ)) {
		// Either an error occurred, or this isn't REG_SZ or REG_EXPAND_SZ.
		if (lpType) {
			*lpType = REG_NONE;
		}
		return {};
	}

	// Save the key type.
	if (lpType) {
		*lpType = dwType;
	}

	// Convert cbData to cchData.
	DWORD cchData = cbData / sizeof(tbuf[0]);

	// Check for NULL terminators.
	for (; cchData > 0; cchData--) {
		if (tbuf[cchData-1] != 0)
			break;
	}

	if (cchData == 0) {
		// No actual string data.
		return {};
	}

	// Return the string.
	return tstring(tbuf.get(), cchData);
}

/**
 * Read a string value from a key. (REG_SZ, REG_EXPAND_SZ)
 * REG_EXPAND_SZ values are expanded if necessary.
 * @param lpValueName	[in] Value name. (Use nullptr or an empty string for the default value.)
 * @param lpType	[out,opt] Variable to store the key type in. (REG_NONE, REG_SZ, or REG_EXPAND_SZ)
 * @return String value, or empty string on error. (check lpType)
 */
tstring RegKey::read_expand(LPCTSTR lpValueName, LPDWORD lpType) const
{
	// Local buffer optimization to reduce memory allocation.
	TCHAR locbuf[128];

	DWORD dwType = 0;
	tstring wstr = read(lpValueName, &dwType);
	if (wstr.empty() || dwType != REG_EXPAND_SZ) {
		// Empty string, or not REG_EXPAND_SZ.
		// Return immediately.
		if (lpType) {
			*lpType = dwType;
		}
		return wstr;
	}

	// Attempt to expand the string using the current environment.
	// cchExpand includes the NULL terminator.
	DWORD cchExpand = ExpandEnvironmentStrings(wstr.c_str(), nullptr, 0);
	if (cchExpand == 0) {
		// Error expanding the strings.
		if (lpType) {
			*lpType = 0;
		}
		return {};
	}

	if (cchExpand <= _countof(locbuf)) {
		// The expanded string fits in the local buffer.
		cchExpand = ExpandEnvironmentStrings(wstr.c_str(), locbuf, cchExpand);
		if (cchExpand == 0) {
			// Error expanding the strings.
			if (lpType) {
				*lpType = 0;
			}
			return {};
		}

		// String has been expanded.
		if (lpType) {
			*lpType = REG_EXPAND_SZ;
		}
		return tstring(locbuf, cchExpand-1);
	}

	// Temporarily allocate a buffer large enough for the string,
	// then call ExpandEnvironmentStrings() again.
	unique_ptr<TCHAR[]> tbuf(new TCHAR[cchExpand]);
	cchExpand = ExpandEnvironmentStrings(wstr.c_str(), tbuf.get(), cchExpand);
	if (cchExpand == 0) {
		// Error expanding the strings.
		if (lpType) {
			*lpType = 0;
		}
		return {};
	}

	// String has been expanded.
	if (lpType) {
		*lpType = REG_EXPAND_SZ;
	}
	return tstring(tbuf.get(), cchExpand-1);
}

/**
 * Read a DWORD value from a key. (REG_DWORD)
 * @param lpValueName	[in] Value name. (Use nullptr or an empty string for the default value.)
 * @param lpType	[out,opt] Variable to store the key type in. (REG_NONE or REG_DWORD)
 * @return DWORD value, or 0 on error. (check lpType)
 */
DWORD RegKey::read_dword(LPCTSTR lpValueName, LPDWORD lpType) const
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
LONG RegKey::write(LPCTSTR lpValueName, LPCTSTR value, DWORD dwType)
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
		// and multiply by sizeof(TCHAR).
		cbData = static_cast<DWORD>((_tcslen(value) + 1) * sizeof(TCHAR));
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
LONG RegKey::write(LPCTSTR lpValueName, const tstring& value, DWORD dwType)
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
	// and multiply by sizeof(TCHAR).
	const DWORD cbData = static_cast<DWORD>((value.size() + 1) * sizeof(TCHAR));

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
LONG RegKey::write_dword(LPCTSTR lpValueName, DWORD value)
{
	if (!m_hKey) {
		// Handle is invalid.
		return ERROR_INVALID_HANDLE;
	}

	DWORD cbData = static_cast<DWORD>(sizeof(value));
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
LONG RegKey::deleteValue(LPCTSTR lpValueName)
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
LONG RegKey::deleteSubKey(HKEY hKeyRoot, LPCTSTR lpSubKey)
{
	// Reference: https://docs.microsoft.com/en-us/windows/win32/sysinfo/deleting-a-key-with-subkeys
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
	unique_ptr<TCHAR[]> szName(new TCHAR[cMaxSubKeyLen]);

	// Enumerate the keys.
	DWORD cchName = cMaxSubKeyLen;
	lResult = RegEnumKeyEx(hSubKey,
		0,		// dwIndex
		szName.get(),	// lpName
		&cchName,	// lpcName
		nullptr,	// lpReserved
		nullptr,	// lpClass
		nullptr,	// lpcClass
		nullptr);	// lpftLastWriteTime
	if (lResult == ERROR_SUCCESS) {
		do {
			// Recurse through this subkey.
			lResult = deleteSubKey(hSubKey, szName.get());
			if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
				break;

			// Next subkey.
			cchName = cMaxSubKeyLen;
			lResult = RegEnumKeyEx(hSubKey,
				0,		// dwIndex
				szName.get(),	// lpName
				&cchName,	// lpcName
				nullptr,	// lpReserved
				nullptr,	// lpClass
				nullptr,	// lpcClass
				nullptr);	// lpftLastWriteTime
		} while (lResult == ERROR_SUCCESS);
	}
	RegCloseKey(hSubKey);

	// Try to delete the key again.
	szName.reset();
	return RegDeleteKey(hKeyRoot, lpSubKey);
}

/**
 * Enumerate subkeys.
 * @param lstSubKeys List to place the subkey names in.
 * @return ERROR_SUCCESS on success; WinAPI error on error.
 */
LONG RegKey::enumSubKeys(forward_list<tstring> &lstSubKeys)
{
	if (!m_hKey) {
		// Handle is invalid.
		return ERROR_INVALID_HANDLE;
	}

	LONG lResult;
	DWORD cSubKeys, cMaxSubKeyLen;

	// Get the number of subkeys.
	lResult = RegQueryInfoKey(m_hKey,
		nullptr, nullptr,		// lpClass, lpcClass
		nullptr,			// lpReserved
		&cSubKeys, &cMaxSubKeyLen,	// lpcSubKeys, lpcMaxSubKeyLen
		nullptr, nullptr,		// lpcMaxClassLen, lpcValues
		nullptr, nullptr,		// lpcMaxValueNameLen, lpcMaxValueLen
		nullptr, nullptr);		// lpcbSecurityDescriptor, lpftLastWriteTime
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// cMaxSubKeyLen doesn't include the NULL terminator.
	cMaxSubKeyLen++;

	// https://docs.microsoft.com/en-us/windows/win32/sysinfo/registry-element-size-limits
	// says key names are limited to 255 characters, but who knows...
	unique_ptr<TCHAR[]> szName(new TCHAR[cMaxSubKeyLen]);

	// Clear the list.
	lstSubKeys.clear();

	for (int i = 0; i < static_cast<int>(cSubKeys); i++) {
		DWORD cchName = cMaxSubKeyLen;
		lResult = RegEnumKeyEx(m_hKey, i,
			szName.get(),	// lpName
			&cchName,	// lpcName
			nullptr,	// lpReserved
			nullptr,	// lpClass
			nullptr,	// lpcClass
			nullptr);	// lpftLastWriteTime
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}

		// Add the subkey name to the return list.
		// cchName contains the number of characters in the
		// subkey name, NOT including the NULL terminator.
		lstSubKeys.emplace_front(szName.get(), cchName);
	}

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
LONG RegKey::RegisterFileType(LPCTSTR fileType, RegKey **pHkey_Assoc)
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
 * Register a COM object in a DLL.
 * @param hInstance DLL to be used for registration
 * @param rclsid CLSID
 * @param progID ProgID
 * @param description Description of the COM object
 * @return ERROR_SUCCESS on success; WinAPI error on error.
 */
LONG RegKey::RegisterComObject(HINSTANCE hInstance, REFCLSID rclsid,
	LPCTSTR progID, LPCTSTR description)
{
	TCHAR szClsid[40];
	LONG lResult = StringFromGUID2(rclsid, szClsid, _countof(szClsid));
	if (lResult <= 0)
		return ERROR_INVALID_PARAMETER;

	// Open HKCR\CLSID.
	RegKey hkcr_CLSID(HKEY_CLASSES_ROOT, _T("CLSID"), KEY_WRITE, false);
	if (!hkcr_CLSID.isOpen())
		return hkcr_CLSID.lOpenRes();

	// Create a key using the CLSID.
	RegKey hkcr_Obj_CLSID(hkcr_CLSID, szClsid, KEY_WRITE, true);
	if (!hkcr_Obj_CLSID.isOpen())
		return hkcr_Obj_CLSID.lOpenRes();
	// Set the default value to the descirption of the COM object.
	lResult = hkcr_Obj_CLSID.write(nullptr, description);
	if (lResult != ERROR_SUCCESS)
		return lResult;

#ifndef NDEBUG
	// Debug build: Disable process isolation to make debugging easier.
	lResult = hkcr_Obj_CLSID.write_dword(_T("DisableProcessIsolation"), 1);
	if (lResult != ERROR_SUCCESS)
		return lResult;
#else
	// Release build: Enable process isolation for increased robustness.
	lResult = hkcr_Obj_CLSID.deleteValue(_T("DisableProcessIsolation"));
	if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
		return lResult;
#endif

	// Create an InprocServer32 subkey.
	RegKey hkcr_InprocServer32(hkcr_Obj_CLSID, _T("InprocServer32"), KEY_WRITE, true);
	if (!hkcr_InprocServer32.isOpen()) {
		return hkcr_InprocServer32.lOpenRes();
	}

	// Set the default value to the filename of the specified DLL.
	// TODO: Duplicated from win32/. Consolidate the two?
	TCHAR dll_filename[MAX_PATH];
	SetLastError(ERROR_SUCCESS);	// required for XP
	DWORD dwResult = GetModuleFileName(hInstance, dll_filename, _countof(dll_filename));
	if (dwResult == 0 || dwResult >= _countof(dll_filename) || GetLastError() != ERROR_SUCCESS) {
		// Cannot get the DLL filename.
		// TODO: Windows XP doesn't SetLastError() if the
		// filename is too big for the buffer.
		lResult = GetLastError();
		if (lResult == ERROR_SUCCESS) {
			lResult = ERROR_INSUFFICIENT_BUFFER;
		}
		return lResult;
	} else {
		lResult = hkcr_InprocServer32.write(nullptr, dll_filename);
		if (lResult != ERROR_SUCCESS)
			return lResult;
	}

	// Set the threading model to Apartment.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/shell/reg-shell-exts
	lResult = hkcr_InprocServer32.write(_T("ThreadingModel"), _T("Apartment"));
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// Create a ProgID subkey.
	RegKey hkcr_Obj_CLSID_ProgID(hkcr_Obj_CLSID, _T("ProgID"), KEY_WRITE, true);
	if (!hkcr_Obj_CLSID_ProgID.isOpen())
		return hkcr_Obj_CLSID_ProgID.lOpenRes();
	// Set the default value.
	return hkcr_Obj_CLSID_ProgID.write(nullptr, progID);
}

/**
 * Register a shell extension as an approved extension.
 * @param rclsid CLSID
 * @param description Description of the shell extension
 * @return ERROR_SUCCESS on success; WinAPI error on error.
 */
LONG RegKey::RegisterApprovedExtension(REFCLSID rclsid, LPCTSTR description)
{
	TCHAR szClsid[40];
	LONG lResult = StringFromGUID2(rclsid, szClsid, _countof(szClsid));
	if (lResult <= 0)
		return ERROR_INVALID_PARAMETER;

	// Open the approved shell extensions key.
	// NOTE: This key might not exist on ReactOS, so we should
	// create it if it's not there.
	RegKey hklm_Approved(HKEY_LOCAL_MACHINE,
		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"),
		KEY_WRITE, true);
	if (!hklm_Approved.isOpen())
		return hklm_Approved.lOpenRes();

	// Create a value for the specified CLSID.
	return hklm_Approved.write(szClsid, description);
}

/**
 * Unregister a COM object in this DLL.
 * @param rclsid CLSID
 * @param progID ProgID
 * @return ERROR_SUCCESS on success; WinAPI error on error.
 */
LONG RegKey::UnregisterComObject(REFCLSID rclsid, LPCTSTR progID)
{
	((void)progID);

	TCHAR szClsid[40];
	LONG lResult = StringFromGUID2(rclsid, szClsid, _countof(szClsid));
	if (lResult <= 0)
		return ERROR_INVALID_PARAMETER;

	// Open HKCR\CLSID.
	RegKey hkcr_CLSID(HKEY_CLASSES_ROOT, _T("CLSID"), KEY_WRITE, false);
	if (!hkcr_CLSID.isOpen())
		return hkcr_CLSID.lOpenRes();

        // Delete the CLSID key.
	// NOTE: deleteSubKey() may return ERROR_FILE_NOT_FOUND,
	// which indicates the object was already unregistered.
	lResult = hkcr_CLSID.deleteSubKey(szClsid);
	if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
		return lResult;

	// Open the approved shell extensions key.
	RegKey hklm_Approved(HKEY_LOCAL_MACHINE,
		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"),
		KEY_WRITE, false);
	if (!hklm_Approved.isOpen())
		return hklm_Approved.lOpenRes();

	// Remove the value for the specified CLSID.
	lResult = hklm_Approved.deleteValue(szClsid);
	if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
		return lResult;

	// TODO: Check progID and remove CLSID if it's present.
	return ERROR_SUCCESS;
}

}
