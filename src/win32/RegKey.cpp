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
 * @param root Root key.
 * @param path Path of the registry key.
 * @param samDesired Desired access rights.
 * @param create If true, create the key if it doesn't exist.
 */
RegKey::RegKey(HKEY root, LPCTSTR path, REGSAM samDesired, bool create)
	: m_lOpenRes(ERROR_SUCCESS)
	, m_hKey(nullptr)
	, m_samDesired(samDesired)
{
	if (create) {
		// REG_CREATE
		m_lOpenRes = RegCreateKeyEx(root, path, 0, nullptr, 0,
			samDesired, nullptr, &m_hKey, nullptr);
	} else {
		// REG_OPEN
		m_lOpenRes = RegOpenKeyEx(root, path, 0, samDesired, &m_hKey);
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

/**
 * Write a value to this key.
 * @param name Value name. (Use nullptr or an empty string for the default value.)
 * @param value Value.
 * @return RegSetValueEx() return value.
 */
LONG RegKey::write(LPCTSTR name, LPCTSTR value)
{
	if (m_hKey == nullptr) {
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
		cbData = (wcslen(value) + 1) * sizeof(wchar_t);
	}

	return RegSetValueEx(m_hKey, name, 0, REG_SZ,
		reinterpret_cast<const BYTE*>(value), cbData);
}

/**
 * Write a value to this key.
 * @param name Value name. (Use nullptr or an empty string for the default value.)
 * @param value Value.
 * @return RegSetValueEx() return value.
 */
LONG RegKey::write(LPCTSTR name, const std::wstring& value)
{
	if (m_hKey == nullptr) {
		// Handle is invalid.
		return ERROR_INVALID_HANDLE;
	}

	// Get the string length, add 1 for NULL,
	// and multiply by sizeof(wchar_t).
	DWORD cbData = (value.size() + 1) * sizeof(wchar_t);

	return RegSetValueEx(m_hKey, name, 0, REG_SZ,
		reinterpret_cast<const BYTE*>(value.c_str()), cbData);
}
