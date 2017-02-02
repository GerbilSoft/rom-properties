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

#ifndef __ROMPROPERTIES_WIN32_REGKEY_HPP__
#define __ROMPROPERTIES_WIN32_REGKEY_HPP__

#include "common.h"

// C++ includes.
#include <string>
#include <list>

class RegKey
{
	public:
		/**
		 * Create or open a registry key.
		 * @param hKeyRoot Root key.
		 * @param path Path of the registry key.
		 * @param samDesired Desired access rights.
		 * @param create If true, create the key if it doesn't exist.
		 */
		RegKey(HKEY hKeyRoot, LPCWSTR path, REGSAM samDesired, bool create = false);

		/**
		 * Create or open a registry key.
		 * @param root Root key.
		 * @param path Path of the registry key.
		 * @param samDesired Desired access rights.
		 * @param create If true, create the key if it doesn't exist.
		 */
		RegKey(const RegKey& root, LPCWSTR path, REGSAM samDesired, bool create = false);

		~RegKey();

	private:
		RP_DISABLE_COPY(RegKey)

	public:
		/**
		 * Get the handle to the opened registry key.
		 * @return Handle to the opened registry key, or INVALID_HANDLE_VALUE if not open.
		 */
		HKEY handle(void) const;

		/**
		* Was the key opened successfully?
		* @return True if the key was opened successfully; false if not.
		*/
		bool isOpen(void) const;

		/**
		* Get the return value of RegCreateKeyEx() or RegOpenKeyEx().
		* @return Return value.
		*/
		LONG lOpenRes(void) const;

		/**
		 * Get the key's desired access rights.
		 * @return Desired access rights.
		 */
		REGSAM samDesired(void) const;

		/**
		 * Close the key.
		 */
		void close(void);

	public:
		/** Basic registry access functions. **/

		/**
		 * Read a string value from a key. (REG_SZ, REG_EXPAND_SZ)
		 * NOTE: REG_EXPAND_SZ values are NOT expanded.
		 * @param lpValueName	[in] Value name. (Use nullptr or an empty string for the default value.)
		 * @param lpType	[out,opt] Variable to store the key type in. (REG_NONE, REG_SZ, or REG_EXPAND_SZ)
		 * @return String value, or empty string on error. (check lpType)
		 */
		std::wstring read(LPCWSTR lpValueName, LPDWORD lpType = nullptr) const;

		/**
		 * Read a DWORD value from a key. (REG_DWORD)
		 * @param lpValueName	[in] Value name. (Use nullptr or an empty string for the default value.)
		 * @param lpType	[out,opt] Variable to store the key type in. (REG_NONE or REG_DWORD)
		 * @return DWORD value, or 0 on error. (check lpType)
		 */
		DWORD read_dword(LPCWSTR lpValueName, LPDWORD lpType = nullptr) const;

		/**
		 * Write a string value to this key.
		 * @param lpValueName Value name. (Use nullptr or an empty string for the default value.)
		 * @param value Value.
		 * @param dwType Key type. (REG_SZ or REG_EXPAND_SZ)
		 * @return RegSetValueEx() return value.
		 */
		LONG write(LPCWSTR lpValueName, LPCWSTR value, DWORD dwType = REG_SZ);

		/**
		 * Write a string value to this key.
		 * @param lpValueName Value name. (Use nullptr or an empty string for the default value.)
		 * @param value Value.
		 * @param dwType Key type. (REG_SZ or REG_EXPAND_SZ)
		 * @return RegSetValueEx() return value.
		 */
		LONG write(LPCWSTR lpValueName, const std::wstring& value, DWORD dwType = REG_SZ);

		/**
		 * Write a DWORD value to this key.
		 * @param lpValueName Value name. (Use nullptr or an empty string for the default value.)
		 * @param value Value.
		 * @return RegSetValueEx() return value.
		 */
		LONG write_dword(LPCWSTR lpValueName, DWORD value);

		/**
		 * Delete a value.
		 * @param lpValueName Value name. (Use nullptr or an empty string for the default value.)
		 * @return RegDeleteValue() return value.
		 */
		LONG deleteValue(LPCWSTR lpValueName);

		/**
		 * Recursively delete a subkey.
		 * @param hKeyRoot Root key.
		 * @param lpSubKey Subkey name.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG deleteSubKey(HKEY hKeyRoot, LPCWSTR subKey);

		/**
		 * Recursively delete a subkey.
		 * @param lpSubKey Subkey name.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		LONG deleteSubKey(LPCWSTR subKey);

		/**
		 * Enumerate subkeys.
		 * @param lstSubKeys List to place the subkey names in.
		 * @return ERROR_SUCCESS on success; WinAPI error on error.
		 */
		LONG enumSubKeys(std::list<std::wstring> &vSubKeys);

		/**
		 * Is the key empty?
		 * This means no values, an empty default value, and no subkey.
		 * @return True if the key is empty; false if not or if an error occurred.
		 */
		bool isKeyEmpty(void);

	public:
		/** COM registration convenience functions. **/

		/**
		* Register a file type.
		* @param fileType File extension, with leading dot. (e.g. ".bin")
		* @param pHkey_Assoc Pointer to RegKey* to store opened registry key on success. (If nullptr, key will be closed.)
		* @return ERROR_SUCCESS on success; WinAPI error on error.
		*/
		static LONG RegisterFileType(LPCWSTR fileType, RegKey **pHkey_Assoc);

		/**
		 * Register a COM object in this DLL.
		 * @param rclsid CLSID.
		 * @param progID ProgID.
		 * @param description Description of the COM object.
		 * @return ERROR_SUCCESS on success; WinAPI error on error.
		 */
		static LONG RegisterComObject(REFCLSID rclsid, LPCWSTR progID, LPCWSTR description);

		/**
		 * Register a shell extension as an approved extension.
		 * @param rclsid CLSID.
		 * @param description Description of the shell extension.
		 * @return ERROR_SUCCESS on success; WinAPI error on error.
		 */
		static LONG RegisterApprovedExtension(REFCLSID rclsid, LPCWSTR description);

		/**
		 * Unregister a COM object in this DLL.
		 * @param rclsid CLSID.
		 * @param progID ProgID.
		 * @return ERROR_SUCCESS on success; WinAPI error on error.
		 */
		static LONG UnregisterComObject(REFCLSID rclsid, LPCWSTR progID);

	protected:
		HKEY m_hKey;		// Registry key handle.
		LONG m_lOpenRes;	// Result from RegOpenKeyEx() or RegCreateKeyEx().
		REGSAM m_samDesired;
};

#endif /* __ROMPROPERTIES_WIN32_REGKEY_HPP__ */
