/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NES.hpp: Nintendo Entertainment System/Famicom ROM reader.              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(NES)
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGEXT()
ROMDATA_DECL_END()

}
