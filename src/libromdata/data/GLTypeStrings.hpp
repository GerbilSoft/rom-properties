/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GLTypeStrings.hpp: OpenGL string tables.                                *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_GLTYPESTRINGS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_GLTYPESTRINGS_HPP__

#include "librpbase/config.librpbase.h"
#include "librpbase/common.h"

// C includes.
#include <stdint.h>

namespace LibRomData {

class GLTypeStrings
{
	private:
		// Static class.
		GLTypeStrings();
		~GLTypeStrings();
		RP_DISABLE_COPY(GLTypeStrings)

	public:
		/**
		 * Look up an OpenGL glType string.
		 * @param glType	[in] glType
		 * @return String, or nullptr if not found.
		 */
		static const char *lookup_glType(unsigned int glType);

		/**
		 * Look up an OpenGL glFormat string.
		 * @param glFormat	[in] glFormat
		 * @return String, or nullptr if not found.
		 */
		static const char *lookup_glFormat(unsigned int glFormat);

		/**
		 * Look up an OpenGL glInternalFormat string.
		 * @param glInternalFormat	[in] glInternalFormat
		 * @return String, or nullptr if not found.
		 */
		static const char *lookup_glInternalFormat(unsigned int glInternalFormat);

		/**
		 * Look up an OpenGL glBaseInternalFormat string.
		 * @param glBaseInternalFormat	[in] glBaseInternalFormat
		 * @return String, or nullptr if not found.
		 */
		static const char *lookup_glBaseInternalFormat(unsigned int glBaseInternalFormat);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_GLTYPESTRINGS_HPP__ */
