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

// C++ includes.
#include <string>

class RegKey
{
	public:
		/**
		 * Create or open a registry key.
		 * @param root Root key.
		 * @param path Path of the registry key.
		 * @param samDesired Desired access rights.
		 * @param create If true, create the key if it doesn't exist.
		 */
		RegKey(HKEY root, LPCTSTR path, REGSAM samDesired, bool create = false);

		/**
		 * Create or open a registry key.
		 * @param root Root key.
		 * @param path Path of the registry key.
		 * @param samDesired Desired access rights.
		 * @param create If true, create the key if it doesn't exist.
		 */
		RegKey(const RegKey& root, LPCTSTR path, REGSAM samDesired, bool create = false);

		~RegKey();

	private:
		RegKey(const RegKey &);
		RegKey &operator=(const RegKey &);

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
		 * Write a value to this key.
		 * @param name Value name. (Use nullptr or an empty string for the default value.)
		 * @param value Value.
		 * @return RegSetValueEx() return value.
		 */
		LONG write(LPCTSTR name, LPCTSTR value);

		/**
		 * Write a value to this key.
		 * @param name Value name. (Use nullptr or an empty string for the default value.)
		 * @param value Value.
		 * @return RegSetValueEx() return value.
		 */
		LONG write(LPCTSTR name, const std::wstring& value);

	public:
		/** COM registration convenience functions. **/

		/**
		 * Register a file type.
		 * @param fileType File extension, with leading dot. (e.g. ".bin")
		 * @param progID ProgID.
		 * @return ERROR_SUCCESS on success; WinAPI error on error.
		 */
		static LONG registerFileType(LPCWSTR fileType, LPCWSTR progID);

		/**
		 * Register a COM object in this DLL.
		 * @param rclsid CLSID.
		 * @param progID ProgID.
		 * @param description Description of the COM object.
		 * @return ERROR_SUCCESS on success; WinAPI error on error.
		 */
		static LONG registerComObject(REFCLSID rclsid, LPCWSTR progID, LPCWSTR description);

		/**
		 * Register a shell extension as an approved extension.
		 * @param rclsid CLSID.
		 * @param description Description of the shell extension.
		 * @return ERROR_SUCCESS on success; WinAPI error on error.
		 */
		static LONG registerApprovedExtension(REFCLSID rclsid, LPCWSTR description);

	protected:
		HKEY m_hKey;		// Registry key handle.
		LONG m_lOpenRes;	// Result from RegOpenKeyEx() or RegCreateKeyEx().
		REGSAM m_samDesired;
};

#endif /* __ROMPROPERTIES_WIN32_REGKEY_HPP__ */
