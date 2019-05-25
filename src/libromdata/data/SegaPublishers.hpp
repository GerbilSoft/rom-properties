/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SegaPublishers.hpp: Sega third-party publishers list.                   *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
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
