/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpTextureWrapper.hpp: librptexture file format wrapper.                 *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_OTHER_RPTEXTUREWRAPPER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_OTHER_RPTEXTUREWRAPPER_HPP__

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(RpTextureWrapper)
ROMDATA_DECL_CLOSE()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_OTHER_RPTEXTUREWRAPPER_HPP__ */
