/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DSSysTitles.hpp: Nintendo 3DS system title lookup.             *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_NINTENDO3DSSYSTITLES_HPP__
#define __ROMPROPERTIES_LIBROMDATA_NINTENDO3DSSYSTITLES_HPP__

#include "common.h"

// C includes.
#include <stdint.h>

namespace LibRomData { namespace Nintendo3DSSysTitles {

/**
 * Look up a Nintendo 3DS system title.
 * @param tid_hi	[in] Title ID High
 * @param tid_lo	[in] Title ID Low
 * @param pRegion	[out,opt] Output buffer for pointer to region name
 * @return System title name, or nullptr on error.
 */
const char8_t *lookup_sys_title(uint32_t tid_hi, uint32_t tid_lo, const char **pRegion);

} }

#endif /* __ROMPROPERTIES_LIBROMDATA_NINTENDO3DSSYSTITLES_HPP__ */
