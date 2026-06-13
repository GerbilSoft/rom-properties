/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * formatting.hpp: Text formatting functions                               *
 *                                                                         *
 * Copyright (c) 2009-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "dll-macros.h"

// C includes (C++ namespace)
#include <cstddef>	/* size_t */
#include <cstdint>

// C++ includes
#include <string>

namespace LibRpText {

/** User Setting Query Function **/

enum class UserSetting {
	Unknown = -1,

	BinaryUnitDialect,

	Max
};

/**
 * User setting query function for LibRpText.
 * @param user_data	[in] User data from registerNotifyFunction()
 * @param setting	[in] User setting to retrieve
 * @return User setting on success; -1 on error.
 */
typedef int (*UserSettingQuery_t)(void *user_data, UserSetting setting);

/**
 * Set the user setting query function.
 * This is used for the UI frontends.
 * @param func Notification function
 * @param user_data User data
 */
RP_LIBROMDATA_PUBLIC
void setUserSettingQueryFunction(UserSettingQuery_t func, void *user_data);

/**
 * Unregister a user setting query function if set.
 *
 * If both func and user_data match the existing values,
 * then both are cleared.
 *
 * @param func Notification function
 * @param user_data User data
 */
RP_LIBROMDATA_PUBLIC
void clearUserSettingQuery(UserSettingQuery_t func, void *user_data);

/** File size formatting **/

enum class BinaryUnitDialect {
	DefaultBinaryDialect = -1,

	IECBinaryDialect,
	JEDECBinaryDialect,
	MetricBinaryDialect,
};

/**
 * Format a file size.
 * @param fileSize File size
 * @param dialect
 * @return Formatted file size.
 */
std::string formatFileSize(off64_t fileSize, BinaryUnitDialect dialect = BinaryUnitDialect::DefaultBinaryDialect);

/**
 * Format a file size, in KiB.
 *
 * This function expects the size to be a multiple of 1024,
 * so it doesn't do any fractional rounding or printing.
 *
 * @param size File size
 * @param dialect
 * @return Formatted file size.
 */
std::string formatFileSizeKiB(unsigned int size, BinaryUnitDialect dialect = BinaryUnitDialect::DefaultBinaryDialect);

/**
 * Format a frequency.
 * @param frequency Frequency.
 * @return Formatted frequency.
 */
std::string formatFrequency(uint32_t frequency);

/** Audio functions **/

/**
 * Format a sample value as m:ss.cs.
 * @param sample Sample value.
 * @param rate Sample rate.
 * @return m:ss.cs
 */
std::string formatSampleAsTime(unsigned int sample, unsigned int rate);

/**
 * Convert a sample value to milliseconds.
 * @param sample Sample value.
 * @param rate Sample rate.
 * @return Milliseconds.
 */
unsigned int convSampleToMs(unsigned int sample, unsigned int rate);

}
