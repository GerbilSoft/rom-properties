/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * GLenumStrings.hpp: OpenGL string tables.                                *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_DATA_GLENUMSTRINGS_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_DATA_GLENUMSTRINGS_HPP__

#include "common.h"

namespace LibRpTexture {

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

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_DATA_GLENUMSTRINGS_HPP__ */
