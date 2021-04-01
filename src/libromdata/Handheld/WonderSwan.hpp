/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WonderSwan.hpp: Bandai WonderSwan (Color) ROM reader.                   *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_WONDERSWAN_HPP__
#define __ROMPROPERTIES_LIBROMDATA_WONDERSWAN_HPP__

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(WonderSwan)
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGEXT()
ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_WONDERSWAN_HPP__ */
