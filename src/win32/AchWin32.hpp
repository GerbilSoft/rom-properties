/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * AchWin32.hpp: Win32 notifications for achievements.                     *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

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
	~AchWin32();

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

public:
	/**
	 * Are any achievement popups still active?
	 * This is needed in order to determine if the DLL can be unloaded.
	 * @return True if any popups are still active; false if not.
	 */
	bool isAnyPopupStillActive(void) const;
};
