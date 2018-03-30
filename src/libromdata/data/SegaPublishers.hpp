/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SegaPublishers.hpp: Sega third-party publishers list.                   *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_SEGAPUBLISHERS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_SEGAPUBLISHERS_HPP__

#include "librpbase/common.h"

namespace LibRomData {

class SegaPublishers
{
	private:
		SegaPublishers();
		~SegaPublishers();
		RP_DISABLE_COPY(SegaPublishers)

	public:
		/**
		 * Look up a company code.
		 * @param code Company code.
		 * @return Publisher, or nullptr if not found.
		 */
		static const char *lookup(unsigned int code);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_SEGAPUBLISHERS_HPP__ */
