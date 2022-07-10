/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AmiiboData.hpp: Nintendo amiibo identification data.                    *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_AMIIBODATA_HPP__
#define __ROMPROPERTIES_LIBROMDATA_AMIIBODATA_HPP__

#include "common.h"

// C includes.
#include <stdint.h>

namespace LibRomData {

class AmiiboDataPrivate;
class AmiiboData
{
	protected:
		/**
		 * AmiiboData class.
		 *
		 * This class is a Singleton, so the caller must obtain a
		 * pointer to the class using instance().
		 */
		AmiiboData();
		~AmiiboData();

	private:
		RP_DISABLE_COPY(AmiiboData)
	private:
		friend class AmiiboDataPrivate;
		AmiiboDataPrivate *d_ptr;

	public:
		/**
		 * Get the AmiiboData instance.
		 *
		 * This automatically initializes the object and
		 * reloads amiibo.bin if it has been modified.
		 *
		 * @return AmiiboData instance.
		 */
		static AmiiboData *instance(void);

	public:
		/**
		 * Look up a character series name.
		 * @param char_id Character ID (Page 21) [must be host-endian]
		 * @return Character series name, or nullptr if not found.
		 */
		const char8_t *lookup_char_series_name(uint32_t char_id) const;

		/**
		 * Look up a character's name.
		 * @param char_id Character ID (Page 21) [must be host-endian]
		 * @return Character name (If variant, the variant name is used.)
		 * If an invalid character ID or variant, nullptr is returned.
		 */
		const char8_t *lookup_char_name(uint32_t char_id) const;

		/**
		 * Look up an amiibo series name.
		 * @param amiibo_id amiibo ID (Page 22) [must be host-endian]
		 * @return Series name, or nullptr if not found.
		 */
		const char8_t *lookup_amiibo_series_name(uint32_t amiibo_id) const;

		/**
		 * Look up an amiibo's series identification.
		 * @param amiibo_id	[in] amiibo ID (Page 22) [must be host-endian]
		 * @param pReleaseNo	[out,opt] Release number within series
		 * @param pWaveNo	[out,opt] Wave number within series
		 * @return amiibo series name, or nullptr if not found.
		 */
		const char8_t *lookup_amiibo_series_data(uint32_t amiibo_id, int *pReleaseNo, int *pWaveNo) const;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_AMIIBODATA_HPP__ */
