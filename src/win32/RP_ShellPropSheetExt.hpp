/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt.hpp: IShellPropSheetExt implementation.            *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_HPP__
#define __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_HPP__

#include "libromdata/config.libromdata.h"
#include "libromdata/common.h"

// References:
// - http://www.codeproject.com/Articles/338268/COM-in-C
// - https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
// - https://msdn.microsoft.com/en-us/library/ms677109(v=vs.85).aspx
#include "RP_ComBase.hpp"

namespace LibRomData {
	class RomData;
}

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_ShellPropSheetExt;
}

// C++ includes.
#include <string>

class RegKey;
class RP_ShellPropSheetExt_Private;

class UUID_ATTR("{2443C158-DF7C-4352-B435-BC9F885FFD52}")
RP_ShellPropSheetExt : public RP_ComBase2<IShellExtInit, IShellPropSheetExt>
{
	public:
		RP_ShellPropSheetExt();
		virtual ~RP_ShellPropSheetExt();

	private:
		typedef RP_ComBase2<IShellExtInit, IShellPropSheetExt> super;
		RP_DISABLE_COPY(RP_ShellPropSheetExt)
	private:
		friend class RP_ShellPropSheetExt_Private;
		RP_ShellPropSheetExt_Private *const d_ptr;

	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) override final;

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
		static LONG RegisterFileType_int(RegKey &hkey_Assoc);

		/**
		 * Unregister the file type handler.
		 *
		 * Internal version; this only unregisters for a single Classes key.
		 * Called by the public version multiple times if a ProgID is registered.
		 *
		 * @param hkey_Assoc File association key to unregister under.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterFileType_int(RegKey &hkey_Assoc);

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
		static LONG RegisterFileType(RegKey &hkcr, LPCWSTR ext);

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
		static LONG UnregisterFileType(RegKey &hkcr, LPCWSTR ext);

	public:
		// IShellExtInit
		IFACEMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID) override final;

		// IShellPropSheetExt
		IFACEMETHODIMP AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam) override final;
		IFACEMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam) override final;

	protected:
		// Property sheet callback functions.
		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static UINT CALLBACK CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

		// Subclass procedure for ES_MULTILINE EDIT controls.
		static LRESULT CALLBACK MultilineEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

		/**
		 * Animated icon timer.
		 * @param hWnd
		 * @param uMsg
		 * @param idEvent
		 * @param dwTime
		 */
		static void CALLBACK AnimTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

		/**
		 * Dialog procedure for subtabs.
		 * @param hWnd
		 * @param uMsg
		 * @param wParam
		 * @param lParam
		 */
		static INT_PTR CALLBACK SubtabDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ShellPropSheetExt, 0x2443c158, 0xdf7c, 0x4352, 0xb4, 0x35, 0xbc, 0x9f, 0x88, 0x5f, 0xfd, 0x52)
#endif

#endif /* __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_HPP__ */
