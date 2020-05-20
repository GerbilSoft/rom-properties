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
		 * Notification function.
		 * @param user_data User data from registerNotifyFunction().
		 * @param desc Achievement description.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		typedef int (RP_C_API *NotifyFunc)(intptr_t user_data, const char *desc);

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

			Max
		};

		/**
		 * Unlock an achievement.
		 * @param id Achievement ID.
		 */
		void unlock(ID id);
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_ACHIEVEMENTS_HPP__ */
