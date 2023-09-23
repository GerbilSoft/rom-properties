/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * KhronosKTX.hpp: Khronos KTX image reader.                               *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "FileFormat.hpp"

namespace LibRpTexture {

FILEFORMAT_DECL_BEGIN(KhronosKTX)
FILEFORMAT_DECL_MIPMAP()

	public:
		static int isRomSupported_static(const DetectInfo *info);

FILEFORMAT_DECL_END()

}
