/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * AchGDBus.hpp: GDBus notifications for achievements.                     *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_ACHGDBUS_HPP__
#define __ROMPROPERTIES_GTK_ACHGDBUS_HPP__

#include "common.h"
#include "librpbase/Achievements.hpp"

class AchGDBusPrivate;
class AchGDBus
{
	protected:
		/**
		 * AchGDBus class.
		 *
		 * This class is a Singleton, so the caller must obtain a
		 * pointer to the class using instance().
		 */
		AchGDBus();
		virtual ~AchGDBus();

	private:
		RP_DISABLE_COPY(AchGDBus);
	private:
		friend class AchGDBusPrivate;
		AchGDBusPrivate *const d_ptr;

	public:
		/**
		 * Get the AchGDBus instance.
		 *
		 * This automatically initializes librpbase's Achievement
		 * object and reloads the achievements data if it has been
		 * modified.
		 *
		 * @return AchGDBus instance.
		 */
		static AchGDBus *instance(void);

		/**
		 * Load the specified Achievements icon sprite sheet.
		 * Caller must free it after use.
		 * @param gray If true, load the grayscale version.
		 * @param iconSize Icon size. (16, 24, 32, 64)
		 * @return PIMGTYPE, or nullptr on error.
		 */
		static PIMGTYPE loadSpriteSheet(int iconSize, bool gray = false);
};

#endif /* __ROMPROPERTIES_GTK_ACHGDBUS_HPP__ */
