/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt.hpp: IShellPropSheetExt implementation.            *
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

#ifndef __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_HPP__
#define __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_HPP__

// References:
// - http://www.codeproject.com/Articles/338268/COM-in-C
// - https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
// - https://msdn.microsoft.com/en-us/library/ms677109(v=vs.85).aspx

#include "RP_ComBase.hpp"
#include "libromdata/config.libromdata.h"
#include "libromdata/RomFields.hpp"

namespace LibRomData {
	class RomData;
}

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_ShellPropSheetExt;
}

// C++ includes.
#include <string>

// Win32 dialog builder.
#include "DialogBuilder.hpp"

class UUID_ATTR("{2443C158-DF7C-4352-B435-BC9F885FFD52}")
RP_ShellPropSheetExt : public RP_ComBase2<IShellExtInit, IShellPropSheetExt>
{
	public:
		RP_ShellPropSheetExt();
		virtual ~RP_ShellPropSheetExt();
	private:
		typedef RP_ComBase2<IShellExtInit, IShellPropSheetExt> super;

	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) override;

	public:
		/**
		 * Register the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG Register(void);

		/**
		 * Unregister the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG Unregister(void);

 	protected:
		// Information from IShellExtInit.
		PCIDLIST_ABSOLUTE m_pidlFolder;
		IDataObject *m_pdtobj;
		HKEY m_hkeyProgID;

		// Selected file.
		wchar_t m_szSelectedFile[MAX_PATH];
		PCWSTR GetSelectedFile(void) const;

		// Dialog builder.
		DialogBuilder m_dlgBuilder;

		// ROM data.
		LibRomData::RomData *m_romData;

		/**
		 * Initialize the dialog for the open ROM data object.
		 * @return Dialog template from DialogBuilder on success; nullptr on error.
		 */
		LPCDLGTEMPLATE initDialog(void);

		/**
		 * Initialize a bitfield layout.
		 * @param hDlg Dialog window.
		 * @param pt_start Starting position, in pixels.
		 * @param idx Field index.
		 */
		void initBitfield(HWND hDlg, const POINT &pt_start, int idx);

		/**
		 * Initialize a ListView control.
		 * @param hWnd HWND of the ListView control.
		 * @param desc RomFields description.
		 * @param data RomFields data.
		 */
		void initListView(HWND hWnd, const LibRomData::RomFields::Desc *desc, const LibRomData::RomFields::Data *data);

		/**
		 * Initialize various fields in a dialog.
		 * @param hDlg Dialog window.
		 */
		void initFields(HWND hDlg);

	public:
		// IShellExtInit
		IFACEMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID);

		// IShellPropSheetExt
		IFACEMETHODIMP AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
		IFACEMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam);

		// Property sheet callback functions.
		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static UINT CALLBACK CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ShellPropSheetExt, 0x2443c158, 0xdf7c, 0x4352, 0xb4, 0x35, 0xbc, 0x9f, 0x88, 0x5f, 0xfd, 0x52)
#endif

#endif /* __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_HPP__ */
