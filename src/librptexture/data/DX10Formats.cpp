/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * DX10Formats.cpp: DirectX 10 formats.                                    *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DX10Formats.hpp"
#include "../fileformat/dds_structs.h"

namespace LibRpTexture { namespace DX10Formats {

#include "DX10Formats_data.h"

/** Public functions **/

/**
 * Look up a DirectX 10 format value.
 * @param dxgiFormat	[in] DXGI_FORMAT
 * @return String, or nullptr if not found.
 */
const char8_t *lookup_dxgiFormat(unsigned int dxgiFormat)
{
	const char8_t *texFormat = nullptr;
	if (dxgiFormat < ARRAY_SIZE(dxgiFormat_offtbl)) {
		const unsigned int offset = dxgiFormat_offtbl[dxgiFormat];
		return (likely(offset != 0) ? &dxgiFormat_strtbl[offset] : nullptr);
	}

	switch (dxgiFormat) {
		case DXGI_FORMAT_FORCE_UINT:
			texFormat = U8("FORCE_UINT");
			break;

		// FAKE formats.
		// These aren't used by actual DX10 DDSes, but *are* used
		// internally by rom-properties for some FourCCs that don't
		// have corresponding DXGI_FORMAT values.
		case DXGI_FORMAT_FAKE_PVRTC_2bpp:
			texFormat = U8("PVRTC 2bpp RGBA");
			break;
		case DXGI_FORMAT_FAKE_PVRTC_4bpp:
			texFormat = U8("PVRTC 4bpp RGBA");
			break;

		default:
			break;
	}

	return texFormat;
}

} }
