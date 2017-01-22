/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AmiiboData.hpp: Nintendo amiibo identification data.                    *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_AMIIBODATA_HPP__
#define __ROMPROPERTIES_LIBROMDATA_AMIIBODATA_HPP__

#include "config.libromdata.h"
#include "common.h"

namespace LibRomData {

class AmiiboData
{
	private:
		// Static class.
		AmiiboData();
		~AmiiboData();
		RP_DISABLE_COPY(AmiiboData)

	public:
		/**
		 * Look up a character series name.
		 * @param char_id Character ID. (Page 21) [must be host-endian]
		 * @return Character series name, or nullptr if not found.
		 */
		static const rp_char *lookup_char_series_name(uint32_t char_id);

		/**
		 * Look up a character's name.
		 * @param char_id Character ID. (Page 21) [must be host-endian]
		 * @return Character name. (If variant, the variant name is used.)
		 * If an invalid character ID or variant, nullptr is returned.
		 */
		static const rp_char *lookup_char_name(uint32_t char_id);

		/**
		 * Look up an amiibo series name.
		 * @param amiibo_id	[in] amiibo ID. (Page 22) [must be host-endian]
		 * @return Series name, or nullptr if not found.
		 */
		static const rp_char *lookup_amiibo_series_name(uint32_t amiibo_id);

		/**
		 * Look up an amiibo's series identification.
		 * @param amiibo_id	[in] amiibo ID. (Page 22) [must be host-endian]
		 * @param pReleaseNo	[out,opt] Release number within series.
		 * @param pWaveNo	[out,opt] Wave number within series.
		 * @return amiibo series name, or nullptr if not found.
		 */
		static const rp_char *lookup_amiibo_series_data(uint32_t amiibo_id, int *pReleaseNo, int *pWaveNo);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_AMIIBODATA_HPP__ */
