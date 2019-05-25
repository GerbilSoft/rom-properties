/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiWIBN.hpp: Nintendo Wii save file banner reader.                      *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_WIIWIBN_HPP__
#define __ROMPROPERTIES_LIBROMDATA_WIIWIBN_HPP__

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(WiiWIBN)
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_ICONANIM()
ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_WIIWIBN_HPP__ */
