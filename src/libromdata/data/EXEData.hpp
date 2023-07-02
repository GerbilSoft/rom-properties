/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXEData.hpp: DOS/Windows executable data.                               *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>

namespace LibRomData { namespace EXEData {

/**
 * Look up a PE machine type. (CPU)
 * @param cpu PE machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char *lookup_pe_cpu(uint16_t cpu);

/**
 * Look up an LE machine type. (CPU)
 * @param cpu LE machine type.
 * @return Machine type name, or nullptr if not found.
 */
const char *lookup_le_cpu(uint16_t cpu);

/**
 * Look up a PE subsystem name.
 * NOTE: This function returns localized subsystem names.
 * @param subsystem PE subsystem
 * @return PE subsystem name, or nullptr if invalid.
 */
const char *lookup_pe_subsystem(uint16_t subsystem);

} }
