/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * Achievements.cpp: Achievements class.                                   *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Achievements.hpp"
#include "libi18n/i18n.h"

namespace LibRpBase {

/**
 * Get the translated description of an achievement.
 * @param id Achievement ID.
 * @return Translated description, or nullptr on error.
 */
const char *Achievements::description(ID id)
{
	static const char *const ach_desc[] = {
		// tr: ViewedDebugCryptedFile
		NOP_C_("Achievements", "Viewed a debug-encrypted file."),
		NOP_C_("Achievements", "Viewed a non-x86/x64 Windows PE executable."),
		NOP_C_("Achievements", "Viewed a BroadOn format Wii WAD file."),
	};

	assert((int)id >= 0);
	assert((int)id < ARRAY_SIZE(ach_desc));
	if ((int)id < 0 || (int)id > ARRAY_SIZE(ach_desc)) {
		// Out of range.
		return nullptr;
	}

	return dpgettext_expr(RP_I18N_DOMAIN, "Achievements", ach_desc[(int)id]);
}

/**
 * Unlock an achievement.
 * @param id Achievement ID.
 */
void Achievements::unlock(ID id)
{
	// TODO: Register the achievement somewhere so we don't keep
	// showing it over and over again.

	// Show the OS-specific notification.
	unlock_os(id);
}

}
