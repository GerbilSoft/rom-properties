/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt.hpp: IShellPropSheetExt implementation.            *
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

#ifndef __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_HPP__
#define __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_HPP__

// librpbase
#include "librpbase/config.librpbase.h"
#include "librpbase/common.h"

// References:
// - http://www.codeproject.com/Articles/338268/COM-in-C
// - https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
// - https://msdn.microsoft.com/en-us/library/ms677109(v=vs.85).aspx
#include "libwin32common/ComBase.hpp"

namespace LibWin32Common {
	class RegKey;
}

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_ShellPropSheetExt;
}

// C++ includes.
#include <string>

class RP_ShellPropSheetExt_Private;

class UUID_ATTR("{2443C158-DF7C-4352-B435-BC9F885FFD52}")
RP_ShellPropSheetExt : public LibWin32Common::ComBase2<IShellExtInit, IShellPropSheetExt>
{
	public:
		RP_ShellPropSheetExt();
		virtual ~RP_ShellPropSheetExt();

	private:
		typedef LibWin32Common::ComBase2<IShellExtInit, IShellPropSheetExt> super;
		RP_DISABLE_COPY(RP_ShellPropSheetExt)
	private:
		friend class RP_ShellPropSheetExt_Private;
		RP_ShellPropSheetExt_Private *const d_ptr;

	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) final;

	private:
		/**
		 * Register the file type handler.
		 *
		 * Internal version; this only registers for a single Classes key.
		 * Called by the public version multiple times if a ProgID is registered.
		 *
		 * @param hkey_Assoc File association key to register under.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG RegisterFileType_int(LibWin32Common::RegKey &hkey_Assoc);

		/**
		 * Unregister the file type handler.
		 *
		 * Internal version; this only unregisters for a single Classes key.
		 * Called by the public version multiple times if a ProgID is registered.
		 *
		 * @param hkey_Assoc File association key to unregister under.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterFileType_int(LibWin32Common::RegKey &hkey_Assoc);

	public:
		/**
		 * Register the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG RegisterCLSID(void);

		/**
		 * Register the file type handler.
		 * @param hkcr HKEY_CLASSES_ROOT or user-specific classes root.
		 * @param ext File extension, including the leading dot.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG RegisterFileType(LibWin32Common::RegKey &hkcr, LPCWSTR ext);

		/**
		 * Unregister the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterCLSID(void);

		/**
		 * Unregister the file type handler.
		 * @param hkcr HKEY_CLASSES_ROOT or user-specific classes root.
		 * @param ext File extension, including the leading dot.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterFileType(LibWin32Common::RegKey &hkcr, LPCWSTR ext);

	public:
		// IShellExtInit
		IFACEMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID) final;

		// IShellPropSheetExt
		IFACEMETHODIMP AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam) final;
		IFACEMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam) final;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ShellPropSheetExt, 0x2443c158, 0xdf7c, 0x4352, 0xb4, 0x35, 0xbc, 0x9f, 0x88, 0x5f, 0xfd, 0x52)
#endif

#endif /* __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_HPP__ */
