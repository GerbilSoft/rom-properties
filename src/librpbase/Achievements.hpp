/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * Achievements.hpp: Achievements class.                                   *
 *                                                                         *
 * Copyright (c) 2020-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "dll-macros.h"

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
		AchievementsPrivate *d_ptr;

	public:
		/**
		 * Get the Achievements instance.
		 *
		 * This automatically initializes the object and
		 * reloads the achievements data if it has been modified.
		 *
		 * @return Achievements instance.
		 */
		RP_LIBROMDATA_PUBLIC
		static Achievements *instance(void);

	public:
		/**
		 * Achievement identifiers.
		 */
		enum class ID {
			// Debug-encrypted file (devkits)
			ViewedDebugCryptedFile		= 0,

			// Non-x86/x64 PE executable
			// Does not include Xbox 360 executables
			ViewedNonX86PE			= 1,

			// BroadOn WAD file format for Wii
			ViewedBroadOnWADFile		= 2,

			// Sonic & Knuckles locked on to Sonic & Knuckles
			ViewedMegaDriveSKwithSK		= 3,

			// CD-i disc image
			ViewedCDiDiscImage		= 4,

			Max
		};

		// Achievements sprite sheet columns/rows.
		static constexpr unsigned int ACH_SPRITE_SHEET_COLS = 4;
		static constexpr unsigned int ACH_SPRITE_SHEET_ROWS = 4;

	public:
		/**
		 * Notification function.
		 * @param user_data	[in] User data from registerNotifyFunction()
		 * @param id		[in] Achievement ID
		 * @return 0 on success; negative POSIX error code on error.
		 */
		typedef int (RP_C_API *NotifyFunc)(void *user_data, ID id);

		/**
		 * Set the notification function.
		 * This is used for the UI frontends.
		 * @param func Notification function
		 * @param user_data User data
		 */
		RP_LIBROMDATA_PUBLIC
		void setNotifyFunction(NotifyFunc func, void *user_data);

		/**
		 * Unregister a notification function if set.
		 *
		 * If both func and user_data match the existing values,
		 * then both are cleared.
		 *
		 * @param func Notification function
		 * @param user_data User data
		 */
		RP_LIBROMDATA_PUBLIC
		void clearNotifyFunction(NotifyFunc func, void *user_data);

	public:
		/**
		 * Unlock an achievement.
		 * @param id Achievement ID.
		 * @param bit Bitfield index for AT_BITFIELD achievements. (-1 for none)
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int unlock(ID id, int bit = -1);

		/**
		 * Check if an achievement is unlocked.
		 * @param id Achievement ID.
		 * @return UNIX time value if unlocked; -1 if not.
		 */
		RP_LIBROMDATA_PUBLIC
		time_t isUnlocked(ID id) const;

	public:
		/**
		 * Get an achievement name. (localized)
		 * @param id Achievement ID.
		 * @return Achievement description, or nullptr on error.
		 */
		RP_LIBROMDATA_PUBLIC
		const char *getName(ID id) const;

		/**
		 * Get an unlocked achievement description. (localized)
		 * @param id Achievement ID.
		 * @return Unlocked achievement description, or nullptr on error.
		 */
		RP_LIBROMDATA_PUBLIC
		const char *getDescUnlocked(ID id) const;
};

}
