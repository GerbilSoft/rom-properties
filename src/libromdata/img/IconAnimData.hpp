/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IconAnimData.hpp: Icon animation data.                                  *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_IMG_ICONANIMDATA_HPP__
#define __ROMPROPERTIES_LIBROMDATA_IMG_ICONANIMDATA_HPP__

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstring>

namespace LibRomData {

class rp_image;

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
	uint8_t seq_index[MAX_SEQUENCE];

	struct delay_t {
		uint16_t numer;	// Numerator.
		uint16_t denom;	// Denominator.
		// TODO: Keep precalculated milliseconds here?
		int ms;		// Precalculated milliseconds.
	};

	// Array of icon delays.
	// NOTE: These are associated with sequence indexes,
	// not the individual icon frames.
	delay_t delays[MAX_SEQUENCE];

	// Array of icon frames.
	// Check the count field to determine
	// how many frames are actually here.
	// NOTE: Frames may be nullptr, in which case
	// the previous frame should be used.
	const rp_image *frames[MAX_FRAMES];

	IconAnimData()
		: count(0)
		, seq_count(0)
	{
		memset(seq_index, 0, sizeof(seq_index));
		memset(delays, 0, sizeof(delays));
		memset(frames, 0, sizeof(frames));
	}
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_IMG_ICONANIMDATA_HPP__ */
