/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * PropSheetIcon.hpp: Property sheet icon.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_CONFIG_PROPSHEETICON_HPP__
#define __ROMPROPERTIES_WIN32_CONFIG_PROPSHEETICON_HPP__

#include "libromdata/common.h"
#include "libromdata/RpWin32_sdk.h"

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
};

#endif /* __ROMPROPERTIES_WIN32_CONFIG_PROPSHEETICON_HPP__ */
