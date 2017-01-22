/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiSystemMenuVersion.hpp: Nintendo Wii System Menu version list.        *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_WIISYSTEMMENUVERSION_HPP__
#define __ROMPROPERTIES_LIBROMDATA_WIISYSTEMMENUVERSION_HPP__

#include "config.libromdata.h"
#include "common.h"

namespace LibRomData {

class WiiSystemMenuVersion
{
	private:
		WiiSystemMenuVersion();
		~WiiSystemMenuVersion();
	private:
		RP_DISABLE_COPY(WiiSystemMenuVersion)

	private:
		/**
		 * Nintendo Wii System Menu version list.
		 * References:
		 * - http://wiibrew.org/wiki/System_Menu
		 * - http://wiiubrew.org/wiki/Title_database
		 * - https://yls8.mtheall.com/ninupdates/reports.php
		 */
		struct SysVersionList {
			unsigned int version;
			const rp_char *str;
		};
		static const SysVersionList ms_sysVersionList[];

	private:
		/**
		 * Comparison function for bsearch().
		 * @param a
		 * @param b
		 * @return
		 */
		static int compar(const void *a, const void *b);

	public:
		/**
		 * Look up a Wii System Menu version.
		 * @param version Version number.
		 * @return Display version, or nullptr if not found.
		 */
		static const rp_char *lookup(unsigned int version);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_WIISYSTEMMENUVERSION_HPP__ */
