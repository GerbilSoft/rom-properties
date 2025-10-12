/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXE.cpp: DOS/Windows executable reader. (Icon handling)                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "EXE.hpp"

// Other rom-properties libraries
//using namespace LibRpBase;
using namespace LibRpFile;
//using namespace LibRpText;

// Windows icon handler
#include "librptexture/fileformat/ico_structs.h"

// C++ STL classes
using std::unique_ptr;

// Private class
// NOTE: Must be #included after C++ headers due to uvector.h.
#include "EXE_p.hpp"

namespace LibRomData {

/** EXEPrivate **/

/**
 * Get the raw resource data for a specific icon.
 * The highest color-depth icon is selected.
 * @param iconindex	[in] Icon index (positive for zero-based index; negative for resource ID)
 * @param width		[in,opt] Icon width (if 0, gets the largest icon)
 * @param height	[in,opt] Icon height (if 0, gets the largest icon)
 * @param pIconResID	[out,opt] Resource ID of the RT_ICON that was loaded
 * @return Raw resource data, or nullptr if not found.
 */
rp::uvector<uint8_t> EXEPrivate::loadIconResourceData(int iconindex, int width, int height, uint32_t *pIconResID)
{
	if (pIconResID) {
		// Clear the returned icon resource ID initially.
		*pIconResID = 0;
	}

	// Make sure the the resource reader is loaded.
	int ret = loadResourceReader();
	if (ret != 0 || !rsrcReader) {
		// No resources available.
		return {};
	}

	uint16_t type = RT_GROUP_ICON;
	if (exeType == ExeType::NE || exeType == ExeType::COM_NE) {
		// Windows 1.x/2.x executables don't have RT_GROUP_ICON,
		// but do have RT_ICON. If this is Win16, check for
		// RT_GROUP_ICON first, then try RT_ICON.
		// NOTE: Can't simply check based on if it's a 1.x/2.x
		// executable because some EXEs converted to 3.x will
		// still show up as 1.x/2.x.
		if (rsrcReader->has_resource_type(RT_GROUP_ICON)) {
			// We have RT_GROUP_ICON.
		} else if (rsrcReader->has_resource_type(RT_ICON)) {
			// We have RT_ICON.
			type = RT_ICON;
		} else {
			// No icons...
			return {};
		}
	}

	// Get the resource ID.
	int resID;
	if (iconindex == 0) {
		// Default icon
		resID = -1;
	} else if (iconindex > 0) {
		// Positive icon index
		// This is a zero-based index into the RT_GROUP_ICON table.
		resID = rsrcReader->lookup_resource_ID(RT_GROUP_ICON, iconindex);
		if (resID < 0) {
			// Not found.
			return {};
		}
	} else {
		// Negative icon index
		// This is an actual resource ID.
		resID = abs(iconindex);
	}

	// If this is an RT_GROUP_ICON, find the RT_ICON that most closely matches the specified size.
	if (type == RT_GROUP_ICON) {
		IRpFilePtr f_rtGroupIcon = rsrcReader->open(type, resID, -1);
		if (!f_rtGroupIcon) {
			// Unable to open the RT_GROUP_ICON...
			return {};
		}

		GRPICONDIR iconDir;
		size_t size = f_rtGroupIcon->read(&iconDir, sizeof(iconDir));
		if (size != sizeof(iconDir)) {
			// Unable to read the GRPICONDIR.
			return {};
		}
		const uint16_t iconCount = le16_to_cpu(iconDir.idCount);
		if (iconCount == 0) {
			// No icons?
			return {};
		}

		// Zero-size == maximum size
		if (width == 0) {
			width = 32767;
		}
		if (height == 0) {
			height = 32767;
		}

		// Current icon
		uint16_t cur_id = 0;
		uint16_t cur_bitCount = 0;
		int cur_w = 0, cur_h = 0;
		for (unsigned int i = 0; i < iconCount; i++) {
			GRPICONDIRENTRY iconDirEntry;
			size = f_rtGroupIcon->read(&iconDirEntry, sizeof(iconDirEntry));
			if (size != sizeof(iconDirEntry)) {
				// Short read.
				break;
			}

			// Check if this icon is better than the current one.
			// NOTE: Width/height '0' really means 256.
			const int new_w = (iconDirEntry.bWidth != 0) ? iconDirEntry.bWidth : 256;
			const int new_h = (iconDirEntry.bHeight != 0) ? iconDirEntry.bHeight : 256;
			const uint16_t new_bitCount = le16_to_cpu(iconDirEntry.wBitCount);

			// Going by size first. Higher bitcount is only a differentiation for identical sizes.
			if (new_w > cur_w || new_h > cur_h) {
				// New icon is bigger.
				if ((cur_w == 0 && cur_h == 0) || (new_w < width && new_h < height)) {
					// New icon is smaller than the previous icon.
					cur_id = le16_to_cpu(iconDirEntry.nID);
					cur_bitCount = new_bitCount;
					cur_w = new_w;
					cur_h = new_h;
				}
			} else if (new_w == cur_w && new_h == cur_h) {
				// Size is identical.
				// If the bit count is higher, use this icon.
				if (new_bitCount > cur_bitCount) {
					// New icon has a higher color depth than the previous icon.
					cur_id = le16_to_cpu(iconDirEntry.nID);
					cur_bitCount = new_bitCount;
					cur_w = new_w;
					cur_h = new_h;
				}
			} else if (new_w < cur_w || new_h < cur_h) {
				// New icon is smaller.
				// Only switch to this one if it's closer to the requested icon size.
				if ((abs(new_w - width) < abs(cur_w - width)) ||
				    (abs(new_h - height) < abs(cur_h - height)))
				{
					// New icon is closer to the requested icon size.
					cur_id = le16_to_cpu(iconDirEntry.nID);
					cur_bitCount = new_bitCount;
					cur_w = new_w;
					cur_h = new_h;
				}
			}
		}

		if (cur_id == 0) {
			// No icon???
			return {};
		}

		// Use this icon resource.
		type = RT_ICON;
		resID = cur_id;
	}

	// Load the icon resource data.
	IRpFilePtr f_rtIcon = rsrcReader->open(type, resID, -1);
	if (!f_rtIcon) {
		// Unable to open the RT_ICON...
		return {};
	}

	// Sanity check: Icon shouldn't be larger than 4 MB.
	static constexpr off64_t MAX_ICON_SIZE = 4 * 1024 * 1024;
	const off64_t iconSize = f_rtIcon->size();
	assert(iconSize > 0);
	assert(iconSize <= MAX_ICON_SIZE);
	if (iconSize <= 0 || iconSize > MAX_ICON_SIZE) {
		return {};
	}

	rp::uvector<uint8_t> iconData;
	iconData.resize(iconSize);
	size_t size = f_rtIcon->read(iconData.data(), iconData.size());
	if (size != iconData.size()) {
		// Read error.
		return {};
	}

	// Icon data retrieved.
	if (pIconResID) {
		*pIconResID = static_cast<uint32_t>(resID);
	}
	return iconData;
}

/** EXE **/

/**
 * Get the raw resource data for a specific icon.
 * The highest color-depth icon is selected.
 * @param iconindex	[in] Icon index (positive for zero-based index; negative for resource ID)
 * @param width		[in,opt] Icon width (if 0, gets the largest icon)
 * @param height	[in,opt] Icon height (if 0, gets the largest icon)
 * @param pIconResID	[out,opt] Resource ID of the RT_ICON that was loaded
 * @return Raw resource data, or nullptr if not found.
 */
rp::uvector<uint8_t> EXE::loadIconResourceData(int iconindex, int width, int height, uint32_t *pIconResID)
{
	RP_D(EXE);
	return d->loadIconResourceData(iconindex, width, height, pIconResID);
}

} // namespace LibRomData
