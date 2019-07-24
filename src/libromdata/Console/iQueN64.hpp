/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * iQueN64.hpp: iQue Nintendo 64 .cmd reader.                              *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_IQUEN64_HPP__
#define __ROMPROPERTIES_LIBROMDATA_IQUEN64_HPP__

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(iQueN64)
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_IQUEN64_HPP__ */
