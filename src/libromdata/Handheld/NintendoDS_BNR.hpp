/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoDS_BNR.hpp: Nintendo DS icon/title data reader.                 *
 * Handles BNR files and icon/title sections.                              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

class NintendoDS_BNR_Private;
ROMDATA_DECL_BEGIN(NintendoDS_BNR)
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_ICONANIM()
ROMDATA_DECL_END()

}
