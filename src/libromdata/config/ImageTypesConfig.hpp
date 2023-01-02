/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ImageTypesConfig.hpp: Image Types non-templated common functions.       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

/**
 * NOTE: ImageTypesConfig contains non-templated common functions used
 * by the templated TImageTypesConfig class. As such, it is compiled by
 * libromdata and should *not* be compiled by UI frontends.
 */

#include "common.h"
#include "librpbase/RomData.hpp"

// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData { namespace ImageTypesConfig {

/**
 * Get the number of image types that can be configured.
 * @return Image type count.
 */
RP_LIBROMDATA_PUBLIC
unsigned int imageTypeCount(void);

/**
 * Get an image type name.
 * @param imageType Image type ID.
 * @return Image type name, or nullptr if invalid.
 */
RP_LIBROMDATA_PUBLIC
const char *imageTypeName(unsigned int imageType);

/**
 * Get the number of systems that can be configured.
 * @return System count.
 */
RP_LIBROMDATA_PUBLIC
unsigned int sysCount(void);

/**
 * Get a system name.
 * @param sys System ID
 * @return System name, or nullptr if invalid.
 */
RP_LIBROMDATA_PUBLIC
const char *sysName(unsigned int sys);

/**
 * Get a class name.
 * @param sys System ID
 * @return Class name, or nullptr if invalid.
 */
RP_LIBROMDATA_PUBLIC
const char *className(unsigned int sys);

/**
 * Get the supported image types for the specified system.
 * @param sys System ID
 * @return Supported image types, or 0 if invalid.
 */
RP_LIBROMDATA_PUBLIC
uint32_t supportedImageTypes(unsigned int sys);

} }
