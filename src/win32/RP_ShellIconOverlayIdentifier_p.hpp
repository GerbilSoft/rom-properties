/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellIconOverlayIdentifier_p.hpp: IShellIconOverlayIdentifier        *
 * (PRIVATE CLASS)                                                         *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "RP_ShellIconOverlayIdentifier.hpp"

// librpbase, librpfile
#include "librpbase/RomData.hpp"
#include "librpfile/IRpFile.hpp"

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
	typedef HRESULT (STDAPICALLTYPE *pfnSHGetStockIconInfo_t)(_In_ SHSTOCKICONID siid, _In_ UINT uFlags, _Out_ SHSTOCKICONINFO *psii);
	HMODULE hShell32_dll;
	pfnSHGetStockIconInfo_t pfnSHGetStockIconInfo;
};
