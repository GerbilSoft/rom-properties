/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomDataFormat.hpp: Common RomData string formatting functions.          *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "time_r.h"	// for rp_time_t

// Qt includes
#include <QtCore/QString>

/**
 * Format an RFT_DATETIME.
 * @param date_time	[in] Date/Time
 * @param flags		[in] RFT_DATETIME flags
 * @return Formatted RFT_DATETIME, or empty on error.
 */
QString formatDateTime(rp_time_t date_time, unsigned int flags);

/**
 * Format an RFT_DIMENSIONS.
 * @param dimensions	[in] Dimensions
 * @return Formatted RFT_DIMENSIONS, or nullptr on error.
 */
QString formatDimensions(const int dimensions[3]);
