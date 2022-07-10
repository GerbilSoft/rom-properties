/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * DX10Formats.hpp: DirectX 10 formats.                                    *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_DATA_DX10FORMATS_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_DATA_DX10FORMATS_HPP__

#include "common.h"

namespace LibRpTexture { namespace DX10Formats {

/**
 * Look up a DirectX 10 format value.
 * @param dxgiFormat	[in] DXGI_FORMAT
 * @return String, or nullptr if not found.
 */
const char8_t *lookup_dxgiFormat(unsigned int dxgiFormat);

} }

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_DATA_DX10FORMATS_HPP__ */
