/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox360_STFS_ContentType.hpp: Microsoft Xbox 360 STFS Content Type.     *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DATA_XBOX360_STFS_CONTENTTYPE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DATA_XBOX360_STFS_CONTENTTYPE_HPP__

#include <stdint.h>
#include "common.h"

namespace LibRomData { namespace Xbox360_STFS_ContentType {

/**
 * Look up an STFS content type.
 * @param contentType Content type.
 * @return Content type, or nullptr if not found.
 */
const char *lookup(uint32_t contentType);

} }

#endif /* __ROMPROPERTIES_LIBROMDATA_DATA_XBOX360_STFS_CONTENTTYPE_HPP__ */
