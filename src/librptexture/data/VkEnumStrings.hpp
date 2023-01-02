/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * VkEnumStrings.hpp: Vulkan string tables.                                *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

namespace LibRpTexture { namespace VkEnumStrings {

/**
 * Look up a Vulkan VkFormat enum string.
 * @param vkFormat	[in] vkFormat enum
 * @return String, or nullptr if not found.
 */
const char *lookup_vkFormat(unsigned int vkFormat);

} }
