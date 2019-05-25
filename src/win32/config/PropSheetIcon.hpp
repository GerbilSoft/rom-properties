/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * PropSheetIcon.hpp: Property sheet icon.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_CONFIG_PROPSHEETICON_HPP__
#define __ROMPROPERTIES_WIN32_CONFIG_PROPSHEETICON_HPP__

#include "librpbase/common.h"
#include "libwin32common/RpWin32_sdk.h"

class PropSheetIcon
{
	private:
		// Static class.
		PropSheetIcon();
		~PropSheetIcon();
		RP_DISABLE_COPY(PropSheetIcon)
	
	public:
		/**
		 * Get the large property sheet icon.
		 * @return Large property sheet icon, or nullptr on error.
		 */
		static HICON getLargeIcon(void);

		/**
		 * Get the small property sheet icon.
		 * @return Small property sheet icon, or nullptr on error.
		 */
		static HICON getSmallIcon(void);

		/**
		 * Get the 96x96 icon.
		 * @return 96x96 icon, or nullptr on error.
		 */
		static HICON get96Icon(void);
};

#endif /* __ROMPROPERTIES_WIN32_CONFIG_PROPSHEETICON_HPP__ */
