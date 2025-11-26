/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataFormat.hpp: Common RomData string formatting functions.          *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <glib.h>

// C++ includes
#include <string>

G_BEGIN_DECLS

/**
 * Format an RFT_DATETIME.
 * @param date_time	[in] Date/Time
 * @param flags		[in] RFT_DATETIME flags
 * @return Formatted RFT_DATETIME, or nullptr on error. (allocated string; free with g_free)
 */
gchar *
rom_data_format_datetime(time_t date_time, unsigned int flags) G_GNUC_MALLOC;

G_END_DECLS

#ifdef __cplusplus

/**
 * Format an RFT_DIMENSIONS.
 * @param dimensions	[in] Dimensions
 * @return Formatted RFT_DIMENSIONS, or nullptr on error.
 */
std::string
rom_data_format_dimensions(const int dimensions[3]);

/**
 * Format multi-line text for Achievements display using Pango markup.
 * Used for AchievementsTab and Achievements-like RFT_LISTDATA fields.
 * @param text Multi-line text
 * @return Text formatted for Achievements display using Pango markup.
 */
std::string
rom_data_format_RFT_LISTDATA_text_as_achievements(const char *text);

/**
 * Format multi-line text for Achievements display using Pango markup.
 * Used for AchievementsTab and Achievements-like RFT_LISTDATA fields.
 * @param text Multi-line text
 * @return Text formatted for Achievements display using Pango markup.
 */
static inline std::string
rom_data_format_RFT_LISTDATA_text_as_achievements(const std::string &text)
{
	return rom_data_format_RFT_LISTDATA_text_as_achievements(text.c_str());
}

#endif /* __cplusplus */
