/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AchLibNotify.cpp: Achievements class. (libnotify version)               *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Achievements.hpp"

namespace LibRpBase {

/**
 * Unlock an achievement.
 * @param id Achievement ID.
 */
void Achievements::unlock(ID id)
{
	// Dummy implementation. Not actually doing anything.
	RP_UNUSED(id);
}

}
