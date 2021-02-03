/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * AchQtDBus.hpp: QtDBus notifications for achievements.                   *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_ACHQTDBUS_HPP__
#define __ROMPROPERTIES_KDE_ACHQTDBUS_HPP__

#include "librpbase/Achievements.hpp"
#include "common.h"

class AchQtDBusPrivate;
class AchQtDBus
{
	protected:
		/**
		 * AchQtDBus class.
		 *
		 * This class is a Singleton, so the caller must obtain a
		 * pointer to the class using instance().
		 */
		AchQtDBus();
		virtual ~AchQtDBus();

	private:
		RP_DISABLE_COPY(AchQtDBus);
	private:
		friend class AchQtDBusPrivate;
		AchQtDBusPrivate *const d_ptr;

	public:
		/**
		 * Get the AchQtDBus instance.
		 *
		 * This automatically initializes librpbase's Achievement
		 * object and reloads the achievements data if it has been
		 * modified.
		 *
		 * @return AchQtDBus instance.
		 */
		static AchQtDBus *instance(void);
};

#endif /* __ROMPROPERTIES_KDE_ACHQTDBUS_HPP__ */
