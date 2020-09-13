/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IconAnimData.hpp: Icon animation data.                                  *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_IMG_ICONANIMDATA_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IMG_ICONANIMDATA_HPP__

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstring>

// C++ includes.
#include <array>
#include <cstdio>

namespace LibRpTexture {
	class rp_image;
}

namespace LibRpBase {

struct IconAnimData
{
	static const int MAX_FRAMES = 64;
	static const int MAX_SEQUENCE = 64;

	int count;	// Frame count.
	int seq_count;	// Sequence count.

	// Array of icon sequence indexes.
	// Each entry indicates which frame to use.
	// Check the seq_count field to determine
	// how many indexes are actually here.
	std::array<uint8_t, MAX_SEQUENCE> seq_index;

	struct delay_t {
		uint16_t numer;	// Numerator.
		uint16_t denom;	// Denominator.
		// TODO: Keep precalculated milliseconds here?
		int ms;		// Precalculated milliseconds.
	};

	// Array of icon delays.
	// NOTE: These are associated with sequence indexes,
	// not the individual icon frames.
	std::array<delay_t, MAX_SEQUENCE> delays;

	// Array of icon frames.
	// Check the count field to determine
	// how many frames are actually here.
	// NOTE: Frames may be nullptr, in which case
	// the previous frame should be used.
	std::array<LibRpTexture::rp_image*, MAX_FRAMES> frames;

	IconAnimData()
		: count(0)
		, seq_count(0)
	{
		seq_index.fill(0);
		frames.fill(0);

		// MSVC 2010 doesn't support initializer lists,
		// so create a dummy struct.
		static const delay_t zero_delay = {0, 0, 0};
		delays.fill(zero_delay);
	}
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IMG_ICONANIMDATA_HPP__ */
