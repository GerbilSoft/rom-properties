/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ELFData.hpp: Executable and Linkable Format data.                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

// C includes.
#include <stdint.h>

namespace LibRomData { namespace ELFData {

/**
 * Look up an ELF machine type. (CPU)
 * @param cpu ELF machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char *lookup_cpu(uint16_t cpu);

/**
 * Look up an ELF OS ABI.
 * @param osabi ELF OS ABI.
 * @return OS ABI name, or nullptr if not found.
 */
const char *lookup_osabi(uint8_t osabi);

} }
