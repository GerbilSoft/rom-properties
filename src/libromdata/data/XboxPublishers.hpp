/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxPublishers.hpp: Xbox third-party publishers list.                   *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_XBOXPUBLISHERS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_XBOXPUBLISHERS_HPP__

#include "common.h"

// C includes.
#include <stdint.h>

namespace LibRomData {

class XboxPublishers
{
	private:
		// Static class.
		XboxPublishers();
		~XboxPublishers();
		RP_DISABLE_COPY(XboxPublishers)

	public:
		/**
		 * Look up a company code.
		 * @param code Company code.
		 * @return Publisher, or nullptr if not found.
		 */
		static const char *lookup(uint16_t code);

		/**
		 * Look up a company code.
		 * @param code Company code.
		 * @return Publisher, or nullptr if not found.
		 */
		static const char *lookup(const char *code);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_XBOXPUBLISHERS_HPP__ */
