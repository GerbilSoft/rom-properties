/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ImageTypesConfig.hpp: Image Types non-templated common functions.       *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_CONFIG_IMAGETYPESCONFIG_HPP__
#define __ROMPROPERTIES_LIBROMDATA_CONFIG_IMAGETYPESCONFIG_HPP__

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

// Number of image types. (columns)
static const int IMG_TYPE_COUNT = LibRpBase::RomData::IMG_EXT_MAX+1;
// Number of systems. (rows)
static const int SYS_COUNT = 10;

/**
 * Get an image type name.
 * @param imageType Image type ID.
 * @return Image type name, or nullptr if invalid.
 */
RP_LIBROMDATA_PUBLIC
const char *imageTypeName(unsigned int imageType);

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

#endif /* __ROMPROPERTIES_LIBROMDATA_CONFIG_IMAGETYPESCONFIG_HPP__ */
