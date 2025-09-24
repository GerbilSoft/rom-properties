/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiU.hpp: Nintendo Wii U disc image reader.                             *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

class WiiUPackage;

ROMDATA_DECL_BEGIN(WiiU)
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGEXT()

private:
	friend class WiiUPackage;

	/**
	 * Get a list of URLs for an external image type.
	 * Common function used by both WiiU and WiiUPackage.
	 *
	 * A thumbnail size may be requested from the shell.
	 * If the subclass supports multiple sizes, it should
	 * try to get the size that most closely matches the
	 * requested size.
	 *
	 * @param id4		[in]     Game ID (ID4)
	 * @param imageType	[in]     Image type
	 * @param extURLs	[out]    Output vector
	 * @param size		[in,opt] Requested image size. This may be a requested
	 *                               thumbnail size in pixels, or an ImageSizeType
	 *                               enum value.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	static int extURLs_int(const char *id4, ImageType imageType, std::vector<ExtURL> &extURLs, int size);

ROMDATA_DECL_END()

}
