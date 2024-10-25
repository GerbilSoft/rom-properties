/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * PowerVR3.hpp: PowerVR 3.0.0 texture image reader.                       *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "FileFormat.hpp"

namespace LibRpTexture {

FILEFORMAT_DECL_BEGIN(PowerVR3)
FILEFORMAT_DECL_MIPMAP()

public:
	static int isRomSupported_static(const DetectInfo *info);

FILEFORMAT_DECL_END()

}
