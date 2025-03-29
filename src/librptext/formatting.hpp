/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * formatting.hpp: Text formatting functions                               *
 *                                                                         *
 * Copyright (c) 2009-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C includes (C++ namespace)
#include <cstdarg>
#include <cstddef>	/* size_t */
#include <cstdint>

// C++ includes
#include <string>

namespace LibRpText {

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
