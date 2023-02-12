/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpTextureWrapper.hpp: librptexture file format wrapper.                 *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(RpTextureWrapper)
ROMDATA_DECL_CLOSE()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()

		/**
		 * Get the pixel format, e.g. "RGB888" or "DXT1".
		 * NOTE: Exported for use by ImageDecoderTest.
		 * @return Pixel format, or nullptr if unavailable.
		 */
		RP_LIBROMDATA_PUBLIC
		const char *pixelFormat(void) const;

ROMDATA_DECL_END()

}
