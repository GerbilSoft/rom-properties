/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiU.hpp: Nintendo Wii U disc image reader.                             *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_WIIU_HPP__
#define __ROMPROPERTIES_LIBROMDATA_WIIU_HPP__

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(WiiU)
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGEXT()
ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_WIIU_HPP__ */
