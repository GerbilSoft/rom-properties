/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpImageLoader.hpp: Image loader class.                                  *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_IMG_RPIMAGELOADER_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IMG_RPIMAGELOADER_HPP__

#include "common.h"

namespace LibRpFile {
	class IRpFile;
}
namespace LibRpTexture {
	class rp_image;
}

namespace LibRpBase { namespace RpImageLoader {

/**
 * Load an image from an IRpFile.
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
RP_LIBROMDATA_PUBLIC
LibRpTexture::rp_image *load(LibRpFile::IRpFile *file);

} }

#endif /* __ROMPROPERTIES_LIBRPBASE_IMG_RP_IMAGE_LOADER_HPP__ */
