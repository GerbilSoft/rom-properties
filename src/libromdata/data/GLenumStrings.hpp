/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GLenumStrings.hpp: OpenGL string tables.                                *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_GLENUMSTRINGS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_GLENUMSTRINGS_HPP__

#include "librpbase/common.h"

namespace LibRomData {

class GLenumStrings
{
	private:
		// Static class.
		GLenumStrings();
		~GLenumStrings();
		RP_DISABLE_COPY(GLenumStrings)

	public:
		/**
		 * Look up an OpenGL GLenum string.
		 * @param glEnum	[in] glEnum
		 * @return String, or nullptr if not found.
		 */
		static const char *lookup_glEnum(unsigned int glEnum);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_GLENUMSTRINGS_HPP__ */
