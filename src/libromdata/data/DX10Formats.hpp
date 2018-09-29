/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DX10Formats.hpp: DirectX 10 formats.                                    *
 *                                                                         *
 * Copyright (c) 2017-2018 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DX10FORMATS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DX10FORMATS_HPP__

#include "librpbase/common.h"

namespace LibRomData {

class DX10Formats
{
	private:
		// Static class.
		DX10Formats();
		~DX10Formats();
		RP_DISABLE_COPY(DX10Formats)

	public:
		/**
		 * Look up a DirectX 10 format value.
		 * @param dxgiFormat	[in] DXGI_FORMAT
		 * @return String, or nullptr if not found.
		 */
		static const char *lookup_dxgiFormat(unsigned int dxgiFormat);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DX10FORMATS_HPP__ */
