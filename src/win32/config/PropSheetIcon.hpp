/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * PropSheetIcon.hpp: Property sheet icon.                                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "libwin32common/RpWin32_sdk.h"

class PropSheetIconPrivate;
class PropSheetIcon
{
protected:
	/**
	 * PropSheetIcon class.
	 *
	 * This class is a Singleton, so the caller must obtain a
	 * pointer to the class using instance().
	 */
	PropSheetIcon();
	~PropSheetIcon();

private:
	RP_DISABLE_COPY(PropSheetIcon);
private:
	friend class PropSheetIconPrivate;
	PropSheetIconPrivate *const d_ptr;

public:
	/**
	 * Get the PropSheetIcon instance.
	 * @return PropSheetIcon instance.
	 */
	static PropSheetIcon *instance(void);

public:
	/**
	 * Get the large property sheet icon.
	 * @return Large property sheet icon, or nullptr on error.
	 */
	HICON getLargeIcon(void) const;

	/**
	 * Get the small property sheet icon.
	 * @return Small property sheet icon, or nullptr on error.
	 */
	HICON getSmallIcon(void) const;

	/**
	 * Get the 96x96 icon.
	 * @return 96x96 icon, or nullptr on error.
	 */
	HICON get96Icon(void) const;
};
