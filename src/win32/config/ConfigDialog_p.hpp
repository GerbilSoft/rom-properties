/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ConfigDialog_p.hpp: Configuration dialog. (Private class)               *
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

#ifndef __ROMPROPERTIES_WIN32_CONFIG_CONFIGDIALOGPRIVATE_HPP__
#define __ROMPROPERTIES_WIN32_CONFIG_CONFIGDIALOGPRIVATE_HPP__

#include "libromdata/common.h"

namespace LibRomData {
	class Config;
}

class ConfigDialogPrivate
{
	public:
		ConfigDialogPrivate(bool isVista);

	private:
		RP_DISABLE_COPY(ConfigDialogPrivate)

	private:
		bool m_isVista;

	public:
		inline bool isVista(void) const
		{
			return m_isVista;
		}

		// Config instance.
		LibRomData::Config *config;

		/**
		 * Convert a bool value to BST_CHCEKED or BST_UNCHECKED.
		 * @param value Bool value.
		 * @return BST_CHECKED or BST_UNCHECKED.
		 */
		static inline int boolToBstChecked(bool value) {
			return (value ? BST_CHECKED : BST_UNCHECKED);
		}

		/**
		 * Convert BST_CHECKED or BST_UNCHECKED to a bool string.
		 * @param value BST_CHECKED or BST_UNCHECKED.
		 * @return Bool string.
		 */
		static inline const wchar_t *bstCheckedToBoolString(unsigned int value) {
			return (value == BST_CHECKED ? L"true" : L"false");
		}

		// Property for "D pointer".
		// This points to the ConfigDialogPrivate object.
		static const wchar_t D_PTR_PROP[];

		// Property sheet change variables.
		// Used for optimization.
		bool changed_ImageTypes;
		bool changed_Downloads;

		// Image type priorities tab functions.
		void reset_ImageTypes(HWND hDlg);
		void save_ImageTypes(HWND hDlg);
		static INT_PTR CALLBACK DlgProc_ImageTypes(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static UINT CALLBACK CallbackProc_ImageTypes(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

		// Downloads tab functions.
		void reset_Downloads(HWND hDlg);
		void save_Downloads(HWND hDlg);
		static INT_PTR CALLBACK DlgProc_Downloads(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static UINT CALLBACK CallbackProc_Downloads(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

		// Thumbnail Cache tab functions.
		static int clearCacheVista(HWND hDlg);
		static INT_PTR CALLBACK DlgProc_Cache(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static UINT CALLBACK CallbackProc_Cache(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

		// Create Property Sheet.
		static INT_PTR CreatePropertySheet(void);
};

#endif /* __ROMPROPERTIES_WIN32_CONFIG_CONFIGDIALOGPRIVATE_HPP__ */
