/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ColumnProvider_p.hpp: IColumnProvider implementation. (PRIVATE CLASS)*
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "RP_ColumnProvider.hpp"

// librpbase
#include "librpbase/RomData.hpp"

// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ColumnProviderPrivate RP_ColumnProvider_Private

// CLSID
extern const CLSID CLSID_RP_ColumnProvider;

// C++ includes
#include <string>
#include "tcharx.h"

class RP_ColumnProvider_Private
{
public:
	RP_ColumnProvider_Private() = default;
	~RP_ColumnProvider_Private() = default;

private:
	RP_DISABLE_COPY(RP_ColumnProvider_Private)

public:
	// RomData object
	LibRpBase::RomDataPtr romData;
	std::tstring tfilename;

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
	static LONG RegisterFileType_int(LibWin32UI::RegKey &hkey_Assoc);

	/**
	 * Unregister the file type handler.
	 *
	 * Internal version; this only unregisters for a single Classes key.
	 * Called by the public version multiple times if a ProgID is registered.
	 *
	 * @param hkey_Assoc File association key to unregister under.
	 * @return ERROR_SUCCESS on success; Win32 error code on error.
	 */
	static LONG UnregisterFileType_int(LibWin32UI::RegKey &hkey_Assoc);
};
