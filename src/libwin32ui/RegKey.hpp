/***************************************************************************
 * ROM Properties Page shell extension. (libwin32ui)                       *
 * RegKey.hpp: Registry key wrapper.                                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C++ includes
#include <forward_list>
#include <list>
#include <string>

// Windows SDK
#include "RpWin32_sdk.h"

namespace LibWin32UI {

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
		RegKey(HKEY hKeyRoot, LPCTSTR path, REGSAM samDesired, bool create = false);

		/**
		 * Create or open a registry key.
		 * @param root Root key.
		 * @param path Path of the registry key.
		 * @param samDesired Desired access rights.
		 * @param create If true, create the key if it doesn't exist.
		 */
		RegKey(const RegKey& root, LPCTSTR path, REGSAM samDesired, bool create = false);

		~RegKey();

		// Disable copy/assignment constructors.
#if __cplusplus >= 201103L
	public:
		RegKey(const RegKey &) = delete;
		RegKey &operator=(const RegKey &) = delete;
#else /* __cplusplus < 201103L */
	private:
		RegKey(const RegKey &);
		RegKey &operator=(const RegKey &);
#endif /* __cplusplus */

	public:
		/**
		 * Get the handle to the opened registry key.
		 * @return Handle to the opened registry key, or INVALID_HANDLE_VALUE if not open.
		 */
		inline HKEY handle(void) const
		{
			return m_hKey;
		}

		/**
		* Was the key opened successfully?
		* @return True if the key was opened successfully; false if not.
		*/
		inline bool isOpen(void) const
		{
			return (m_hKey != nullptr);
		}

		/**
		* Get the return value of RegCreateKeyEx() or RegOpenKeyEx().
		* @return Return value.
		*/
		inline LONG lOpenRes(void) const
		{
			return m_lOpenRes;
		}

		/**
		 * Get the key's desired access rights.
		 * @return Desired access rights.
		 */
		inline REGSAM samDesired(void) const
		{
			return m_samDesired;
		}

		/**
		 * Close the key.
		 */
		void close(void);

	public:
		/** Basic registry access functions. **/

		/**
		 * Read a string value from a key. (REG_SZ, REG_EXPAND_SZ)
		 * REG_EXPAND_SZ values are NOT expanded.
		 * @param lpValueName	[in] Value name. (Use nullptr or an empty string for the default value.)
		 * @param lpType	[out,opt] Variable to store the key type in. (REG_NONE, REG_SZ, or REG_EXPAND_SZ)
		 * @return String value, or empty string on error. (check lpType)
		 */
		std::tstring read(LPCTSTR lpValueName, LPDWORD lpType = nullptr) const;

		/**
		 * Read a string value from a key. (REG_SZ, REG_EXPAND_SZ)
		 * REG_EXPAND_SZ values are expanded if necessary.
		 * @param lpValueName	[in] Value name. (Use nullptr or an empty string for the default value.)
		 * @param lpType	[out,opt] Variable to store the key type in. (REG_NONE, REG_SZ, or REG_EXPAND_SZ)
		 * @return String value, or empty string on error. (check lpType)
		 */
		std::tstring read_expand(LPCTSTR lpValueName, LPDWORD lpType = nullptr) const;

		/**
		 * Read a DWORD value from a key. (REG_DWORD)
		 * @param lpValueName	[in] Value name. (Use nullptr or an empty string for the default value.)
		 * @param lpType	[out,opt] Variable to store the key type in. (REG_NONE or REG_DWORD)
		 * @return DWORD value, or 0 on error. (check lpType)
		 */
		DWORD read_dword(LPCTSTR lpValueName, LPDWORD lpType = nullptr) const;

		/**
		 * Write a string value to this key.
		 * @param lpValueName Value name. (Use nullptr or an empty string for the default value.)
		 * @param value Value.
		 * @param dwType Key type. (REG_SZ or REG_EXPAND_SZ)
		 * @return RegSetValueEx() return value.
		 */
		LONG write(LPCTSTR lpValueName, LPCTSTR value, DWORD dwType = REG_SZ);

		/**
		 * Write a string value to this key.
		 * @param lpValueName Value name. (Use nullptr or an empty string for the default value.)
		 * @param value Value.
		 * @param dwType Key type. (REG_SZ or REG_EXPAND_SZ)
		 * @return RegSetValueEx() return value.
		 */
		LONG write(LPCTSTR lpValueName, const std::tstring& value, DWORD dwType = REG_SZ);

		/**
		 * Write a DWORD value to this key.
		 * @param lpValueName Value name. (Use nullptr or an empty string for the default value.)
		 * @param value Value.
		 * @return RegSetValueEx() return value.
		 */
		LONG write_dword(LPCTSTR lpValueName, DWORD value);

		/**
		 * Delete a value.
		 * @param lpValueName Value name. (Use nullptr or an empty string for the default value.)
		 * @return RegDeleteValue() return value.
		 */
		LONG deleteValue(LPCTSTR lpValueName);

		/**
		 * Recursively delete a subkey.
		 * @param hKeyRoot Root key.
		 * @param lpSubKey Subkey name.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG deleteSubKey(HKEY hKeyRoot, LPCTSTR lpSubKey);

		/**
		 * Recursively delete a subkey.
		 * @param lpSubKey Subkey name.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		inline LONG deleteSubKey(LPCTSTR lpSubKey)
		{
			return deleteSubKey(m_hKey, lpSubKey);
		}

		/**
		 * Enumerate subkeys.
		 * @param lstSubKeys List to place the subkey names in.
		 * @return ERROR_SUCCESS on success; WinAPI error on error.
		 */
		LONG enumSubKeys(std::forward_list<std::tstring> &vSubKeys);

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
		static LONG RegisterFileType(LPCTSTR fileType, RegKey **pHkey_Assoc);

		/**
		 * Register a COM object in a DLL.
		 * @param hInstance DLL to be used for registration
		 * @param rclsid CLSID
		 * @param progID ProgID
		 * @param description Description of the COM object
		 * @return ERROR_SUCCESS on success; WinAPI error on error.
		 */
		static LONG RegisterComObject(HINSTANCE hInstance, REFCLSID rclsid,
			LPCTSTR progID, LPCTSTR description);

		/**
		 * Register a shell extension as an approved extension.
		 * @param rclsid CLSID
		 * @param description Description of the shell extension
		 * @return ERROR_SUCCESS on success; WinAPI error on error.
		 */
		static LONG RegisterApprovedExtension(REFCLSID rclsid, LPCTSTR description);

		/**
		 * Unregister a COM object.
		 * @param rclsid CLSID
		 * @param progID ProgID
		 * @return ERROR_SUCCESS on success; WinAPI error on error.
		 */
		static LONG UnregisterComObject(REFCLSID rclsid, LPCTSTR progID);

	protected:
		HKEY m_hKey;		// Registry key handle.
		LONG m_lOpenRes;	// Result from RegOpenKeyEx() or RegCreateKeyEx().
		REGSAM m_samDesired;
};

}
