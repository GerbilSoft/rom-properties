/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * Achievements.hpp: Achievements class.                                   *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_ACHIEVEMENTS_HPP__
#define __ROMPROPERTIES_LIBRPBASE_ACHIEVEMENTS_HPP__

#include "common.h"

namespace LibRpTexture {
	class rp_image;
}

namespace LibRpBase {

class AchievementsPrivate;
class Achievements
{
	protected:
		/**
		 * Achievements class.
		 *
		 * This class is a Singleton, so the caller must obtain a
		 * pointer to the class using instance().
		 */
		Achievements();
		~Achievements();

	private:
		RP_DISABLE_COPY(Achievements)
	private:
		friend class AchievementsPrivate;
		AchievementsPrivate *const d_ptr;

	public:
		/**
		 * Get the Achievements instance.
		 *
		 * This automatically initializes the object and
		 * reloads the achievements data if it has been modified.
		 *
		 * @return Achievements instance.
		 */
		static Achievements *instance(void);

	public:
		/**
		 * Achievement identifiers.
		 */
		enum class ID {
			// Debug-encrypted file. (devkits)
			ViewedDebugCryptedFile		= 0,

			// Non-x86/x64 PE executable.
			// Does not include Xbox 360 executables.
			ViewedNonX86PE			= 1,

			// BroadOn WAD file format for Wii.
			ViewedBroadOnWADFile		= 2,

			// Sonic & Knuckles locked on to Sonic & Knuckles
			ViewedMegaDriveSKwithSK		= 3,

			Max
		};

		// Achievements sprite sheet columns/rows.
		static const unsigned int ACH_SPRITE_SHEET_COLS = 4;
		static const unsigned int ACH_SPRITE_SHEET_ROWS = 4;

	public:
		/**
		 * Notification function.
		 * @param user_data	[in] User data from registerNotifyFunction().
		 * @param id		[in] Achievement ID.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		typedef int (RP_C_API *NotifyFunc)(intptr_t user_data, ID id);

		/**
		 * Set the notification function.
		 * This is used for the UI frontends.
		 * @param func Notification function.
		 * @param user_data User data.
		 */
		void setNotifyFunction(NotifyFunc func, intptr_t user_data);

		/**
		 * Unregister a notification function if set.
		 *
		 * If both func and user_data match the existing values,
		 * then both are cleared.
		 *
		 * @param func Notification function.
		 * @param user_data User data.
		 */
		void clearNotifyFunction(NotifyFunc func, intptr_t user_data);

	public:
		/**
		 * Unlock an achievement.
		 * @param id Achievement ID.
		 * @param bit Bitfield index for AT_BITFIELD achievements. (-1 for none)
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int unlock(ID id, int bit = -1);

		/**
		 * Get an achievement name. (localized)
		 * @param id Achievement ID.
		 * @return Achievement description, or nullptr on error.
		 */
		const char *getName(ID id) const;

		/**
		 * Get an unlocked achievement description. (localized)
		 * @param id Achievement ID.
		 * @return Unlocked achievement description, or nullptr on error.
		 */
		const char *getDescUnlocked(ID id) const;
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_ACHIEVEMENTS_HPP__ */
