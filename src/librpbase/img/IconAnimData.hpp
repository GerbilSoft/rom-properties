/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IconAnimData.hpp: Icon animation data.                                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

// C includes
#include <stdint.h>

// C includes (C++ namespace)
#include <cstring>

// C++ includes
#include <algorithm>
#include <array>
#include <cstdio>
#include <memory>

// librptexture
#include "librptexture/img/rp_image.hpp"

namespace LibRpBase {

struct IconAnimData final
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
	std::array<LibRpTexture::rp_image_ptr, MAX_FRAMES> frames;

	IconAnimData()
		: count(0)
		, seq_count(0)
	{
		seq_index.fill(0);
		frames.fill(nullptr);
		delays.fill({0, 0, 0});
	}

private:
	RP_DISABLE_COPY(IconAnimData);
};

typedef std::shared_ptr<IconAnimData> IconAnimDataPtr;
typedef std::shared_ptr<const IconAnimData> IconAnimDataConstPtr;

}
