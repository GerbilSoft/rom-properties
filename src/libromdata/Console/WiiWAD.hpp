/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiWAD.hpp: Nintendo Wii WAD file reader.                               *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_WIIWAD_HPP__
#define __ROMPROPERTIES_LIBROMDATA_WIIWAD_HPP__

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(WiiWAD)
ROMDATA_DECL_CLOSE()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_ICONANIM()
ROMDATA_DECL_IMGEXT()
ROMDATA_DECL_ROMOPS()
ROMDATA_DECL_VIEWED_ACHIEVEMENTS()
ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_WIIWAD_HPP__ */
