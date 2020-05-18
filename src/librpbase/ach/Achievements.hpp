/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * Achievements.hpp: Achievements class.                                   *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_ACH_ACHIEVEMENTS_HPP__
#define __ROMPROPERTIES_LIBRPBASE_ACH_ACHIEVEMENTS_HPP__

#include "common.h"

namespace LibRpBase {

class Achievements
{
	private:
		// Static class.
		Achievements() { }
		~Achievements() { }

	private:
		RP_DISABLE_COPY(Achievements)

	public:
		/**
		 * Achievement identifiers.
		 */
		enum class ID {
			ViewedDebugCryptedFile = 0,
			ViewedNonX86PE,
			ViewedBroadOnWADFile,

			AchievementMax
		};

		/**
		 * Get the translated description of an achievement.
		 * @param id Achievement ID.
		 * @return Translated description, or nullptr on error.
		 */
		static const char *description(ID id);

		/**
		 * Unlock an achievement.
		 * @param id Achievement ID.
		 */
		static void unlock(ID id);

	protected:
		/**
		 * OS-specific achievement notify function.
		 * @param id Achievement ID.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int unlock_os(ID id);
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_ACH_ACHIEVEMENTS_HPP__ */
