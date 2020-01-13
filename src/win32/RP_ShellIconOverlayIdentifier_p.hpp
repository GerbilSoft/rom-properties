/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellIconOverlayIdentifier_p.hpp: IShellIconOverlayIdentifier        *
 * (PRIVATE CLASS)                                                         *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
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
