/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * AchGDBus.hpp: GDBus notifications for achievements.                     *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

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
		~AchGDBus();

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
};
