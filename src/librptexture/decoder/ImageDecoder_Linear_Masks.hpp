/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_Linear_Masks.cpp: Image decoding functions: Linear         *
 * Common masks and other values.                                          *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>

// librptexture
#include "img/rp_image.hpp"

namespace LibRpTexture { namespace ImageDecoder { namespace Masks {

// AND masks for 565 channels.
static constexpr uint16_t Mask565_Hi5  = 0xF800;
static constexpr uint16_t Mask565_Mid6 = 0x07E0;
static constexpr uint16_t Mask565_Lo5  = 0x001F;

// AND masks for 555 channels.
static constexpr uint16_t Mask555_Hi5  = 0x7C00;
static constexpr uint16_t Mask555_Mid5 = 0x03E0;
static constexpr uint16_t Mask555_Lo5  = 0x001F;

// AND masks for 4444 channels.
static constexpr uint16_t Mask4444_Nyb3 = 0xF000;
static constexpr uint16_t Mask4444_Nyb2 = 0x0F00;
static constexpr uint16_t Mask4444_Nyb1 = 0x00F0;
static constexpr uint16_t Mask4444_Nyb0 = 0x000F;

// AND masks for 1555 channels.
static constexpr uint16_t Cmp1555_A     = 0x0080;
static constexpr uint16_t Mask1555_Hi5  = 0x7C00;
static constexpr uint16_t Mask1555_Mid5 = 0x03E0;
static constexpr uint16_t Mask1555_Lo5  = 0x001F;

// AND masks for 5551 channels.
static constexpr uint16_t Cmp5551_A     = 0x0101;
static constexpr uint16_t Mask5551_Hi5  = 0xF800;
static constexpr uint16_t Mask5551_Mid5 = 0x07C0;
static constexpr uint16_t Mask5551_Lo5  = 0x003E;

// Alpha mask
static constexpr uint32_t Mask32_A = 0xFF000000;

// GR88 mask
static constexpr uint32_t MaskGR88 = 0x00FFFF00;

// sBIT metadata
static const rp_image::sBIT_t sBIT_RGB565   = {5,6,5,0,0};
static const rp_image::sBIT_t sBIT_ARGB1555 = {5,5,5,0,1};
static const rp_image::sBIT_t sBIT_xRGB4444 = {4,4,4,0,0};
static const rp_image::sBIT_t sBIT_ARGB4444 = {4,4,4,0,4};
static const rp_image::sBIT_t sBIT_RGB555   = {5,5,5,0,0};

} } }
