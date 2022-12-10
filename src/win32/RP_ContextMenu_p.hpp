/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ContextMenu_p.hpp: IContextMenu implementation. (PRIVATE CLASS)      *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_P_HPP__
#define __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_P_HPP__

#include "RP_ContextMenu.hpp"

// C++ includes
#include <string>
#include <vector>

// TCHAR
#include "tcharx.h"

// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ContextMenuPrivate RP_ContextMenu_Private

// CLSID
extern const CLSID CLSID_RP_ContextMenu;

class RP_ContextMenu_Private
{
	public:
		RP_ContextMenu_Private();
		~RP_ContextMenu_Private();

	private:
		RP_DISABLE_COPY(RP_ContextMenu_Private)

	public:
		// Selected filenames. [from IShellExtInit::Initialize()]
		std::vector<LPTSTR> filenames;

		/**
		 * Clear the filenames vector.
		 */
		void clear_filenames(void);

		/**
		 * Convert a texture file to PNG format.
		 * Destination filename will be generated based on the source filename.
		 * @param source_file Source filename
		 * @return 0 on success; non-zero on error.
		 */
		int convert_to_png(LPCTSTR source_file);

	public:
		/**
		 * Register the file type handler.
		 *
		 * Internal version; this only registers for a single Classes key.
		 * Called by the public version multiple times if a ProgID is registered.
		 *
		 * @param hkey_Assoc File association key to register under.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG RegisterFileType(LibWin32UI::RegKey &hkey_Assoc);

		/**
		 * Unregister the file type handler.
		 *
		 * Internal version; this only unregisters for a single Classes key.
		 * Called by the public version multiple times if a ProgID is registered.
		 *
		 * @param hkey_Assoc File association key to unregister under.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterFileType(LibWin32UI::RegKey &hkey_Assoc);
};

#endif /* __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_P_HPP__ */
