/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox360_STFS.hpp: Microsoft Xbox 360 package reader.                    *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX360_STFS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX360_STFS_HPP__

#include "librpbase/RomData.hpp"

namespace LibRomData {

class Xbox360_STFS_Private;
ROMDATA_DECL_BEGIN(Xbox360_STFS)
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX360_STFS_HPP__ */
