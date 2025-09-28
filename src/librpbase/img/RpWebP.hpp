/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpWebP.hpp: WebP image handler.                                         *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

// Other rom-properties libraries
#include "librpfile/IRpFile.hpp"
#include "librptexture/img/rp_image.hpp"

namespace LibRpBase {

namespace RpWebP {

/**
 * Load a WebP image from an IRpFile.
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
LibRpTexture::rp_image_ptr load(LibRpFile::IRpFile *file);

/**
 * Load a WebP image from an IRpFile.
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
static inline LibRpTexture::rp_image_ptr load(const LibRpFile::IRpFilePtr &file)
{
	return load(file.get());
}

} }
