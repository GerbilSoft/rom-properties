/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * XboxXPR.hpp: Microsoft Xbox XPR0 texture reader.                        *
 *                                                                         *
 * Copyright (c) 2019-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "FileFormat.hpp"

namespace LibRpTexture {

FILEFORMAT_DECL_BEGIN(XboxXPR)
FILEFORMAT_DECL_END()

typedef std::shared_ptr<XboxXPR> XboxXPRPtr;

}
