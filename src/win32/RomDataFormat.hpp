/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RomDataFormat.hpp: Common RomData string formatting functions.          *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <ctime>

// C++ includes
#include <string>

#include "tcharx.h"

/**
 * Format an RFT_DATETIME.
 * @param date_time	[in] Date/Time
 * @param flags		[in] RFT_DATETIME flags
 * @return Formatted RFT_DATETIME, or empty on error.
 */
std::tstring formatDateTime(time_t date_time, unsigned int flags);

/**
 * Format an RFT_DIMENSIONS.
 * @param dimensions	[in] Dimensions
 * @return Formatted RFT_DIMENSIONS, or nullptr on error. (allocated string; free with g_free)
 */
std::tstring formatDimensions(const int dimensions[3]);
