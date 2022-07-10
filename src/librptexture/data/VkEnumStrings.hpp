/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * VkEnumStrings.hpp: Vulkan string tables.                                *
 *                                                                         *
 * Copyright (c) 2019-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_DATA_VKENUMSTRINGS_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_DATA_VKENUMSTRINGS_HPP__

#include "common.h"

namespace LibRpTexture { namespace VkEnumStrings {

/**
 * Look up a Vulkan VkFormat enum string.
 * @param vkFormat	[in] vkFormat enum
 * @return String, or nullptr if not found.
 */
const char8_t *lookup_vkFormat(unsigned int vkFormat);

} }

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_DATA_VKENUMSTRINGS_HPP__ */
