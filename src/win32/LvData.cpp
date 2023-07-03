/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * LvData.cpp: ListView data internal implementation.                      *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "LvData.hpp"

// librpbase
using LibRpBase::RomFields;

// libwin32ui
#include "libwin32ui/AutoGetDC.hpp"
using LibWin32UI::AutoGetDC;

// C++ STL classes.
using std::tstring;
using std::vector;

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
vector<int> LvData::measureColumnWidths(int *p_nl_max) const
{
	vector<int> col_widths;
	int nl_max = 0;
	if (!vvStr.empty()) {
		col_widths.resize(vvStr[0].size());
	}

	AutoGetDC hDC(hListView);

	for (const auto &data_row : vvStr) {
		col_widths.resize(data_row.size());

		int col = 0;
		for (const auto &data_item : data_row) {
			int nl_count = 0;
			const int width = LibWin32UI::measureStringForListView(hDC, data_item, &nl_count);
			col_widths[col] = std::max(col_widths[col], width);
			nl_max = std::max(nl_max, nl_count);

			col++;
		}
	}

	if (p_nl_max) {
		*p_nl_max = nl_max;
	}
	return col_widths;
}

/** Sorting **/

/**
 * Reset the sorting map.
 * This uses the "default" sort.
 */
void LvData::resetSortMap(void)
{
	vSortMap.resize(vvStr.size());
	const unsigned int size = static_cast<unsigned int>(vSortMap.size());
	for (unsigned int i = 0; i < size; i++) {
		vSortMap[i] = i;
	}
}

/**
 * Set the initial sorting setting.
 * @param column Column.
 * @param direction Direction.
 */
void LvData::setInitialSort(int column, RomFields::ColSortOrder direction)
{
	// Get the Header control.
	HWND hHeader = ListView_GetHeader(hListView);
	assert(hHeader != nullptr);
	if (!hHeader) {
		// No Header control...
		return;
	}

	assert(column >= 0);
	if (column < 0) {
		// Invalid column...
		// TODO: Validate upper bound?
		resetSortMap();
		return;
	}

	// Update the header item.
	HDITEM hdi;
	Header_GetItem(hHeader, column, &hdi);
	switch (direction) {
		default:
			assert(!"Invalid sort direction.");
			// fall-through
		case RomFields::COLSORTORDER_ASCENDING:
			hdi.fmt |= HDF_SORTUP;
			break;
		case RomFields::COLSORTORDER_DESCENDING:
			hdi.fmt |= HDF_SORTDOWN;
			break;
	}
	Header_SetItem(hHeader, column, &hdi);

	// Initialize the sort map.
	resetSortMap();

	// Sort the ListView data.
	doSort(column, direction);
}

/**
 * Toggle a sort column.
 * Usually called in response to LVN_COLUMNCLICK.
 * @param iSubItem Column number.
 * @return TRUE if handled; FALSE if not.
 */
BOOL LvData::toggleSortColumn(int iSubItem)
{
	// Get the Header control.
	HWND hHeader = ListView_GetHeader(hListView);
	assert(hHeader != nullptr);
	if (!hHeader) {
		// No Header control...
		return FALSE;
	}

	// Adjust header item states.
	RomFields::ColSortOrder direction = RomFields::COLSORTORDER_ASCENDING;
	const int itemCount = Header_GetItemCount(hHeader);
	for (int i = 0; i < itemCount; i++) {
		HDITEM hdi;
		Header_GetItem(hHeader, i, &hdi);

		if (i == iSubItem) {
			// Set the arrow to Up if not set; flip it if set.
			if (hdi.fmt & HDF_SORTUP) {
				// Currently Up.
				hdi.fmt ^= (HDF_SORTUP | HDF_SORTDOWN);
				direction = RomFields::COLSORTORDER_DESCENDING;
			} else if (hdi.fmt & HDF_SORTDOWN) {
				// Currently Down.
				hdi.fmt ^= (HDF_SORTUP | HDF_SORTDOWN);
			} else {
				// Not set. Set it to Up.
				hdi.fmt |=  HDF_SORTUP;
			}
			Header_SetItem(hHeader, i, &hdi);
		} else {
			// Not the sort column.
			if (hdi.fmt & (HDF_SORTUP | HDF_SORTDOWN)) {
				// Clear the sort indicator.
				hdi.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
				Header_SetItem(hHeader, i, &hdi);
			}
		}
	}

	// Sort the ListView data.
	doSort(iSubItem, direction);
	return TRUE;
}

/**
 * Numeric comparison function.
 * @param strA
 * @param strB
 * @return -1, 0, or 1 (like strcmp())
 */
int LvData::doNumericCompare(LPCTSTR strA, LPCTSTR strB)
{
	int ret = 0;

	// Handle NULL strings as if they're 0.
	// TODO: Allow arbitrary bases?
	// FIXME: Locale-independent functions.
	TCHAR *endptrA = (TCHAR*)_T(""), *endptrB = (TCHAR*)_T("");
	int64_t valA = (strA ? _tcstoi64(strA, &endptrA, 10) : 0);
	int64_t valB = (strB ? _tcstoi64(strB, &endptrB, 10) : 0);

	// If the values match, do a case-insensitive string comparison
	// if the strings didn't fully convert to numbers.
	if (valA == valB) {
		if (*endptrA == _T('\0') && *endptrB == _T('\0')) {
			// Both strings are numbers.
			// No need to do a string comparison.
		} else if (!strA || !strB) {
			if (!strA && !strB) {
				// Both strings are NULL.
				// Handle this as if they're equal.
			} else {
				// Only one string is NULL.
				// That will be sorted before the other string.
				ret = (!strA ? -1 : 1);
			}
		} else {
			// Do a string comparison.
			ret = _tcscmp(strA, strB);
		}
	} else if (valA < valB) {
		ret = -1;
	} else /*if (valA > valB)*/ {
		ret = 1;
	}

	return ret;
}

/**
 * Do a sort.
 * This does NOT adjust the Header control.
 * @param column Sort column.
 * @param direction Sort direction.
 */
void LvData::doSort(int column, RomFields::ColSortOrder direction)
{
	const RomFields::ColSorting method = static_cast<RomFields::ColSorting>(
		(sortingMethods >> (column * RomFields::COLSORT_BITS)) & RomFields::COLSORT_MASK);

	auto compar = [this, column, method](int a, int b) -> bool {
		// TODO: Do we need these checks?
		assert(a >= 0);
		assert(a < (int)vvStr.size());
		assert(b >= 0);
		assert(b < (int)vvStr.size());
		if (a < 0 || a >= (int)vvStr.size() ||
		    b < 0 || b >= (int)vvStr.size())
		{
			return false;
		}

		auto &rowA = vvStr[a];
		auto &rowB = vvStr[b];
		assert(column < (int)rowA.size());
		assert(column < (int)rowB.size());
		if (column >= (int)rowA.size() || column >= (int)rowB.size())
			return false;

		const tstring &strA = rowA[column];
		const tstring &strB = rowB[column];

		bool bRet;
		switch (method) {
			default:
				// Unsupported. We'll use standard sorting.
				assert(!"Unsupported sorting method.");
				// fall-through
			case RomFields::COLSORT_STANDARD:
				// Standard sorting.
				bRet = (strA.compare(strB) < 0);
				break;
			case RomFields::COLSORT_NOCASE: {
				// Case-insensitive sorting.
				bRet = (CompareString(LOCALE_USER_DEFAULT,
					LINGUISTIC_IGNORECASE,
					strA.data(), static_cast<int>(strA.size()),
					strB.data(), static_cast<int>(strB.size())) == CSTR_LESS_THAN);
				break;
			}
			case RomFields::COLSORT_NUMERIC: {
				// Numeric sorting.
				bRet = (doNumericCompare(strA, strB) < 0);
				break;
			}
		}
		return bRet;
	};

	assert(column >= 0);
	if (column < 0)
		return;

	// TODO: Reset the sort map?
	switch (direction) {
		default:
			assert(!"Invalid sort direction.");
			// fall-through
		case RomFields::COLSORTORDER_ASCENDING:
			std::sort(vSortMap.begin(), vSortMap.end(), compar);
			break;
		case RomFields::COLSORTORDER_DESCENDING:
			std::sort(vSortMap.rbegin(), vSortMap.rend(), compar);
			break;
	}

	// Redraw all items.
	ListView_RedrawItems(hListView, 0, static_cast<int>(vSortMap.size()));
}
