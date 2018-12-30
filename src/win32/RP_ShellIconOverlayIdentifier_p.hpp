/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellIconOverlayIdentifier_p.hpp: IShellIconOverlayIdentifier        *
 * (PRIVATE CLASS)                                                         *
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

#ifndef __ROMPROPERTIES_WIN32_RP_SHELLICONOVERLAYIDENTIFIER_P_HPP__
#define __ROMPROPERTIES_WIN32_RP_SHELLICONOVERLAYIDENTIFIER_P_HPP__

#include "RP_ShellIconOverlayIdentifier.hpp"

// librpbase
#include "librpbase/file/IRpFile.hpp"
#include "librpbase/RomData.hpp"

// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ShellIconOverlayIdentifierPrivate RP_ShellIconOverlayIdentifier_Private

// CLSID
extern const CLSID CLSID_RP_ShellIconOverlayIdentifier;

// C++ includes.
#include <vector>

class RP_ShellIconOverlayIdentifier_Private
{
	public:
		RP_ShellIconOverlayIdentifier_Private();
		~RP_ShellIconOverlayIdentifier_Private();

	private:
		RP_DISABLE_COPY(RP_ShellIconOverlayIdentifier_Private)

	public:
		// SHGetStockIconInfo() for the UAC shield icon.
		typedef HRESULT (STDAPICALLTYPE *PFNSHGETSTOCKICONINFO)(_In_ SHSTOCKICONID siid, _In_ UINT uFlags, _Out_ SHSTOCKICONINFO *psii);
		HMODULE hShell32_dll;
		PFNSHGETSTOCKICONINFO pfnSHGetStockIconInfo;
};

#endif /* __ROMPROPERTIES_WIN32_RP_SHELLICONOVERLAYIDENTIFIER_P_HPP__ */
