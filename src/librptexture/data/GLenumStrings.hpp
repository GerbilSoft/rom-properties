/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * GLenumStrings.hpp: OpenGL string tables.                                *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_DATA_GLENUMSTRINGS_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_DATA_GLENUMSTRINGS_HPP__

#include "common.h"

namespace LibRpTexture { namespace GLenumStrings {

/**
 * Look up an OpenGL GLenum string.
 * @param glEnum	[in] glEnum
 * @return String, or nullptr if not found.
 */
const char8_t *lookup_glEnum(unsigned int glEnum);

} }

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_DATA_GLENUMSTRINGS_HPP__ */
