/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox360_STFS_ContentType.hpp: Microsoft Xbox 360 STFS Content Type.     *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DATA_XBOX360_STFS_CONTENTTYPE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DATA_XBOX360_STFS_CONTENTTYPE_HPP__

#include "librpbase/common.h"
#include <stdint.h>

namespace LibRomData {

class Xbox360_STFS_ContentType
{
	private:
		Xbox360_STFS_ContentType();
		~Xbox360_STFS_ContentType();
		RP_DISABLE_COPY(Xbox360_STFS_ContentType)

	public:
		/**
		 * Look up an STFS content type.
		 * @param contentType Content type.
		 * @return Content type, or nullptr if not found.
		 */
		static const char *lookup(uint32_t contentType);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DATA_XBOX360_STFS_CONTENTTYPE_HPP__ */
