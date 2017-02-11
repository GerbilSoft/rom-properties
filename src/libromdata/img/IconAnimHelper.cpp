/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IconAnimHelper.cpp: Icon animation helper.                              *
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

#include "IconAnimHelper.hpp"
#include "rp_image.hpp"
#include "IconAnimData.hpp"
#include <cstdio>
// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData {

IconAnimHelper::IconAnimHelper()
	: m_iconAnimData(nullptr)
	, m_seq_idx(0)
	, m_frame(0)
	, m_delay(0)
	, m_last_valid_frame(0)
{ }

IconAnimHelper::IconAnimHelper(const IconAnimData *iconAnimData)
	: m_iconAnimData(iconAnimData)
	, m_seq_idx(0)
	, m_frame(0)
	, m_delay(0)
	, m_last_valid_frame(0)
{
	reset();
}

/**
 * Set the iconAnimData.
 * @param iconAnimData New iconAnimData.
 */
void IconAnimHelper::setIconAnimData(const IconAnimData *iconAnimData)
{
	m_iconAnimData = iconAnimData;
	reset();
}

/**
 * Get the iconAnimData.
 * @return iconAnimData.
 */
const IconAnimData *IconAnimHelper::iconAnimData(void) const
{
	return m_iconAnimData;
}

/**
 * Is this an animated icon?
 *
 * This checks if iconAnimData is set and has an animation
 * sequence that refers to more than one frame.
 *
 * @return True if this is an animated icon; false if not.
 */
bool IconAnimHelper::isAnimated(void) const
{
	// TODO: Verify that the sequence references more than one frame?
	return (m_iconAnimData &&
		m_iconAnimData->count > 0 &&
		m_iconAnimData->seq_count > 0);
}

/**
 * Get the current frame number.
 *
 * Note that this is actually the last valid frame
 * that had a valid image.
 *
 * @return Frame number.
 */
int IconAnimHelper::frameNumber(void) const
{
	return m_last_valid_frame;
}

/**
 * Get the current frame's delay.
 * @return Current frame's delay, in milliseconds.
 */
int IconAnimHelper::frameDelay(void) const
{
	return m_delay;
}

/**
 * Reset the animation.
 */
void IconAnimHelper::reset(void)
{
	if (m_iconAnimData) {
		assert(m_iconAnimData->count > 1);
		assert(m_iconAnimData->count <= (int)m_iconAnimData->frames.size());
		assert(m_iconAnimData->seq_count > 1);
		assert(m_iconAnimData->seq_count <= (int)m_iconAnimData->seq_index.size());
		m_seq_idx = 0;
		m_frame = m_iconAnimData->seq_index[0];
		m_delay = m_iconAnimData->delays[0].ms;
		m_last_valid_frame = m_frame;
	} else {
		// No animation.
		m_seq_idx = 0;
		m_frame = 0;
		m_delay = 0;
		m_last_valid_frame = 0;
	}
}

/**
 * Advance the animation by one frame.
 * @param pDelay	[out] Pointer to int to store the frame delay, in milliseconds.
 * @return Next frame number. (Returns 0 if there's no animation.)
 */
int IconAnimHelper::nextFrame(int *pDelay)
{
	if (!m_iconAnimData) {
		// No animation data.
		return 0;
	}

	// Go to the next frame in the sequence.
	if (m_seq_idx >= (m_iconAnimData->seq_count - 1)) {
		// Last frame in the sequence.
		m_seq_idx = 0;
	} else {
		// Go to the next frame in the sequence.
		m_seq_idx++;
	}

	// Get the frame number associated with this sequence index.
	m_frame = m_iconAnimData->seq_index[m_seq_idx];
	assert(m_frame >= 0);
	assert(m_frame < (int)m_iconAnimData->frames.size());

	// Get the frame delay. (TODO: Must be > 0?)
	m_delay = m_iconAnimData->delays[m_seq_idx].ms;
	if (pDelay) {
		*pDelay = m_delay;
	}

	// Check if this frame is valid.
	if (m_iconAnimData->frames[m_frame] != nullptr &&
	    m_iconAnimData->frames[m_frame]->isValid())
	{
		// Frame is valid.
		m_last_valid_frame = m_frame;
	}

	return m_last_valid_frame;
}

}
