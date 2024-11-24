/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpImageLoader.hpp: Image loader class.                                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

// Other rom-properties libraries
#include "librpfile/IRpFile.hpp"
#include "librptexture/img/rp_image.hpp"

namespace LibRpBase { namespace RpImageLoader {

/**
 * Load an image from an IRpFile.
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
RP_LIBROMDATA_PUBLIC
LibRpTexture::rp_image_ptr load(LibRpFile::IRpFile *file);

/**
 * Load an image from an IRpFile.
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
LibRpTexture::rp_image_ptr load(const LibRpFile::IRpFilePtr &file)
{
	return load(file.get());
}

} }
