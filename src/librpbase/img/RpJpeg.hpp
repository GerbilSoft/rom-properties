/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpJpeg.hpp: JPEG image handler.                                         *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

namespace LibRpFile {
	class IRpFile;
}
namespace LibRpTexture {
	class rp_image;
}

// C++ includes
#include <memory>

namespace LibRpBase {

class RpJpeg
{
	public:
		// Static class
		RpJpeg() = delete;
		~RpJpeg() = delete;
	private:
		RP_DISABLE_COPY(RpJpeg)

	public:
		/**
		 * Load a JPEG image from an IRpFile.
		 * @param file IRpFile to load from.
		 * @return rp_image*, or nullptr on error.
		 */
		static std::shared_ptr<LibRpTexture::rp_image> load(const LibRpFile::IRpFilePtr &file);
};

}
