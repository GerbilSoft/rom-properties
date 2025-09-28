/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AndroidAPK.hpp: Android APK package reader.                             *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(AndroidAPK)
ROMDATA_DECL_CLOSE()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_END()

}
