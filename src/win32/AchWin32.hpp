/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * AchWin32.hpp: Win32 notifications for achievements.                     *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_ACHWIN32_HPP__
#define __ROMPROPERTIES_WIN32_ACHWIN32_HPP__

#include "librpbase/Achievements.hpp"
#include "common.h"

class AchWin32Private;
class AchWin32
{
	protected:
		/**
		 * AchWin32 class.
		 *
		 * This class is a Singleton, so the caller must obtain a
		 * pointer to the class using instance().
		 */
		AchWin32();
		virtual ~AchWin32();

	private:
		RP_DISABLE_COPY(AchWin32);
	private:
		friend class AchWin32Private;
		AchWin32Private *const d_ptr;

	public:
		/**
		 * Get the AchWin32 instance.
		 *
		 * This automatically initializes librpbase's Achievement
		 * object and reloads the achievements data if it has been
		 * modified.
		 *
		 * @return AchWin32 instance.
		 */
		static AchWin32 *instance(void);
};

#endif /* __ROMPROPERTIES_KDE_ACHQTDBUS_HPP__ */
