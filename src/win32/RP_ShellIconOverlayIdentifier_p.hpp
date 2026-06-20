/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellIconOverlayIdentifier_p.hpp: IShellIconOverlayIdentifier        *
 * (PRIVATE CLASS)                                                         *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "RP_ShellIconOverlayIdentifier.hpp"

// Other rom-properties libraries
#include "librpbase/RomData.hpp"
#include "librpfile/IRpFile.hpp"

// C includes
#include "tcharx.h"

// C++ STL classes
#include <mutex>
#include <string>

// Windows API
#include <shellapi.h>

// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ShellIconOverlayIdentifierPrivate RP_ShellIconOverlayIdentifier_Private

class RP_ShellIconOverlayIdentifier_Private
{
private:
	RP_DISABLE_COPY(RP_ShellIconOverlayIdentifier_Private)

public:
	// UAC shield icon variables
	static std::once_flag uac_once_flag;
	static std::tstring uac_shield_filename;
	static int uac_shield_index;

	/**
	 * Initialize the UAC shield icon variables.
	 *
	 * Internal function; must be called using std::call_once().
	 */
	static void initUacShieldIconVars(void);
};
