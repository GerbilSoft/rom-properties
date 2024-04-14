/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * LvData.hpp: ListView data internal implementation.                      *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

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
	HWND hListView;					// Associated ListView control
	std::vector<std::vector<std::tstring> > vvStr;	// String data
	std::vector<int> vImageList;			// ImageList indexes

	// Sorting: key == display index, value == LvData_t index
	std::vector<unsigned int> vSortMap;

	// Column widths
	std::vector<int> col_widths;

	// For RFT_LISTDATA_MULTI only!
	const LibRpBase::RomFields::Field *pField;

	// Column 0 size adjustment.
	// Used for icon and/or checkbox.
	int col0sizeadj;

	uint32_t checkboxes;				// Checkboxes.
	uint32_t sortingMethods;			// Sorting methods.
	bool hasCheckboxes;				// True if checkboxes are valid.

	explicit LvData(HWND hListView, LibRpBase::RomFields::Field *pField = nullptr)
		: hListView(hListView), pField(pField)
		, col0sizeadj(0), checkboxes(0)
		, sortingMethods(0), hasCheckboxes(false)
	{}

public:
	/** Strings **/

	/**
	 * Measure column widths.
	 * This measures all column widths and doesn't use
	 * LVSCW_AUTOSIZE_USEHEADER.
	 *
	 * For RFT_LISTDATA_MULTI, this uses the currently-loaded
	 * string data.
	 *
	 * @param p_nl_max	[out,opt] Maximum number of newlines found in all entries.
	 * @return Column widths.
	 */
	std::vector<int> measureColumnWidths(int *p_nl_max) const;

public:
	/** Sorting **/

	/**
	 * Reset the sorting map.
	 * This uses the "default" sort.
	 */
	void resetSortMap(void);

	/**
	 * Set the initial sorting setting.
	 * @param column Column.
	 * @param direction Direction.
	 */
	void setInitialSort(int column, LibRpBase::RomFields::ColSortOrder direction);

	/**
	 * Toggle a sort column.
	 * Usually called in response to LVN_COLUMNCLICK.
	 * @param iSubItem Column number.
	 * @return TRUE if handled; FALSE if not.
	 */
	BOOL toggleSortColumn(int iSubItem);

private:
	/**
	 * Numeric comparison function.
	 * @param strA
	 * @param strB
	 * @return -1, 0, or 1 (like strcmp())
	 */
	static int doNumericCompare(LPCTSTR strA, LPCTSTR strB);

	/**
	 * Numeric comparison function.
	 * @param strA
	 * @param strB
	 * @return -1, 0, or 1 (like strcmp())
	 */
	static inline int doNumericCompare(const std::tstring &strA, const std::tstring &strB)
	{
		return doNumericCompare(strA.c_str(), strB.c_str());
	}

	/**
	 * Do a sort.
	 * This does NOT adjust the Header control.
	 * @param column Sort column.
	 * @param direction Sort direction.
	 */
	void doSort(int column, LibRpBase::RomFields::ColSortOrder direction);
};
