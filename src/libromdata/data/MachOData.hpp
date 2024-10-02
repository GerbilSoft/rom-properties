/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MachOData.hpp: Mach-O executable format data.                           *
 *                                                                         *
 * Copyright (c) 2020-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cstdint>

namespace LibRomData { namespace MachOData {

/**
 * Look up a Mach-O CPU type.
 * @param cputype Mach-O CPU type.
 * @return CPU type name, or nullptr if not found.
 */
const char *lookup_cpu_type(uint32_t cputype);

/**
 * Look up a Mach-O CPU subtype.
 * @param cputype Mach-O CPU type.
 * @param cpusubtype Mach-O CPU subtype.
 * @return OS ABI name, or nullptr if not found.
 */
const char *lookup_cpu_subtype(uint32_t cputype, uint32_t cpusubtype);

} }
