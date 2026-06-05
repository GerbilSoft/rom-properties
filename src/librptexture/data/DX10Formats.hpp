/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * DX10Formats.hpp: DirectX 10 formats.                                    *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

namespace LibRpTexture { namespace DX10Formats {

/**
 * Look up a DirectX 10 format value.
 * @param dxgiFormat	[in] DXGI_FORMAT
 * @return String, or nullptr if not found.
 */
const char *lookup_dxgiFormat(unsigned int dxgiFormat);

} }
