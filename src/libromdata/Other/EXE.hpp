/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE.hpp: DOS/Windows executable reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(EXE)
ROMDATA_DECL_DANGEROUS()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_VIEWED_ACHIEVEMENTS()
ROMDATA_DECL_END()

}
