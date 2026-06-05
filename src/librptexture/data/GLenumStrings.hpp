/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * GLenumStrings.hpp: OpenGL string tables.                                *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

namespace LibRpTexture { namespace GLenumStrings {

/**
 * Look up an OpenGL GLenum string.
 * @param glEnum	[in] glEnum
 * @return String, or nullptr if not found.
 */
const char *lookup_glEnum(unsigned int glEnum);

} }
