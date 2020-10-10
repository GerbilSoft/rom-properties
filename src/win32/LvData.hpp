/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * LvData.hpp: ListView data internal implementation.                      *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_RP_LVDATA_HPP__
#define __ROMPROPERTIES_WIN32_RP_LVDATA_HPP__

// librpbase
#include "librpbase/RomFields.hpp"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"

// C++ includes.
#include <string>
#include <vector>

// TCHAR
#include "tcharx.h"

// ListView data struct.
// NOTE: Not making vImageList a pointer, since that adds
// significantly more complexity.
struct LvData {
	std::vector<std::vector<std::tstring> > vvStr;	// String data.
	std::vector<int> vImageList;			// ImageList indexes.
	uint32_t checkboxes;				// Checkboxes.
	bool hasCheckboxes;				// True if checkboxes are valid.

	// Sorting: key == display index, value == LvData_t index
	std::vector<unsigned int> vSortMap;
	uint32_t sortingMethods;			// Sorting methods.

	// For RFT_LISTDATA_MULTI only!
	HWND hListView;
	const LibRpBase::RomFields::Field *pField;

	LvData()
		: checkboxes(0), hasCheckboxes(false)
		, hListView(nullptr), pField(nullptr) { }

	/**
	 * Reset the sorting map.
	 * This uses the "default" sort.
	 */
	void resetSortMap(void);
};

#endif /* __ROMPROPERTIES_WIN32_RP_LVDATA_HPP__ */
