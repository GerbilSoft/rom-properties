/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE.hpp: DOS/Windows executable reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"
#include "uvector.h"

namespace LibRomData {

ROMDATA_DECL_BEGIN_EXPORT_CTOR(EXE)
ROMDATA_DECL_DANGEROUS()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()

public:
	/**
	 * Load a specific icon by index.
	 * @param iconindex Icon index (positive for zero-based index; negative for resource ID)
	 * @return Icon, or nullptr if not found.
	 */
	LibRpTexture::rp_image_const_ptr loadSpecificIcon(int iconindex);

	/**
	 * Get the raw resource data for a specific icon.
	 * The highest color-depth icon is selected.
	 * @param iconindex	[in] Icon index (positive for zero-based index; negative for resource ID)
	 * @param width		[in,opt] Icon width (if 0, gets the largest icon)
	 * @param height	[in,opt] Icon height (if 0, gets the largest icon)
	 * @param pIconResID	[out,opt] Resource ID of the RT_ICON that was loaded
	 * @return Raw resource data, or nullptr if not found.
	 */
	RP_LIBROMDATA_PUBLIC
	rp::uvector<uint8_t> loadIconResourceData(int iconindex, int width = 0, int height = 0, uint32_t *pIconResID = nullptr);

ROMDATA_DECL_VIEWED_ACHIEVEMENTS()
ROMDATA_DECL_END()

}
