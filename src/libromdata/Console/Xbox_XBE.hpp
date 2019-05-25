/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox_XBE.hpp: Microsoft Xbox executable reader.                         *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX_XBE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX_XBE_HPP__

#include "librpbase/config.librpbase.h"
#include "librpbase/RomData.hpp"

namespace LibRomData {

class Xbox_XBE_Private;
ROMDATA_DECL_BEGIN(Xbox_XBE)
ROMDATA_DECL_CLOSE()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX_XBE_HPP__ */
