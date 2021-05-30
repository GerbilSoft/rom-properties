/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDrive.hpp: Sega Mega Drive ROM reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_MEGADRIVE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_MEGADRIVE_HPP__

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(MegaDrive)
ROMDATA_DECL_VIEWED_ACHIEVEMENTS()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGEXT()
ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_MEGADRIVE_HPP__ */
