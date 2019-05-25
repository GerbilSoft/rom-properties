/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SegaPVR.hpp: Sega PVR image reader.                                     *
 *                                                                         *
 * Copyright (c) 2017-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_SEGAPVR_HPP__
#define __ROMPROPERTIES_LIBROMDATA_SEGAPVR_HPP__

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(SegaPVR)
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_SEGAPVR_HPP__ */
