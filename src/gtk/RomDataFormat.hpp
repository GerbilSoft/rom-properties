/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataFormat.hpp: Common RomData string formatting functions.          *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_ROMDATAFORMAT_HPP__
#define __ROMPROPERTIES_GTK_ROMDATAFORMAT_HPP__

#include <glib.h>

G_BEGIN_DECLS

/**
 * Format an RFT_DATETIME.
 * @param date_time	[in] Date/Time
 * @param flags		[in] RFT_DATETIME flags
 * @return Formatted RFT_DATETIME, or nullptr on error. (allocated string; free with g_free)
 */
gchar *
rom_data_format_datetime(time_t date_time, unsigned int flags) G_GNUC_MALLOC;

/**
 * Format an RFT_DIMENSIONS.
 * @param dimensions	[in] Dimensions
 * @return Formatted RFT_DIMENSIONS, or nullptr on error. (allocated string; free with g_free)
 */
gchar *
rom_data_format_dimensions(const int dimensions[3]) G_GNUC_MALLOC;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_ROMDATAFORMAT_HPP__ */
