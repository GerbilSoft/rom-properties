/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * ListDataSortProxyModel.cpp: QSortFilterProxyModel for RFT_LISTDATA.     *
 *                                                                         *
 * Copyright (c) 2012-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include <stdafx.h>
#include "ListDataSortProxyModel.hpp"

// librpbase
#include "librpbase/RomFields.hpp"
using LibRpBase::RomFields;

/**
 * Parse a QString as a number.
 * @param str		[in] QString
 * @param pbAllNumeric	[out,opt] Set to true if the entire QString is numeric.
 * @return Number
 */
qlonglong ListDataSortProxyModel::parseQString(const QString &str, bool *pbAllNumeric)
{
	int pos = 0;
	const int str_size = str.size();
	for (; pos < str_size; pos++) {
		if (!str[pos].isDigit())
			break;
	}

	bool isAllNumeric = (pos == str.size());
	if (pbAllNumeric) {
		*pbAllNumeric = isAllNumeric;
	}

	if (isAllNumeric) {
		// Entire string is numeric.
		return str.toLongLong();
	} else if (pos == 0) {
		// None of the string is numeric.
		return 0;
	}

	// Only part of the string is numeric.
	return str.mid(0, pos).toLongLong();
}

/**
 * Numeric comparison function.
 * @param text1
 * @param text2
 * @return True if text1 < text2
 */
bool ListDataSortProxyModel::doNumericCompare(const QString &text1, const QString &text2)
{
	if (text1.isEmpty() && text2.isEmpty()) {
		// Both strings are empty.
		return false;
	}

	bool ok1 = false, ok2 = false;
	qlonglong val1 = parseQString(text1, &ok1);
	qlonglong val2 = parseQString(text2, &ok2);

	if (val1 == val2) {
		// Values are identical.
		if (ok1 && !ok2) {
			// Second string is not fully numeric.
			// text1 < text2
			return true;
		} else {
			// Other cases: text1 >= text2
			return false;
		}
	}

	return (val1 < val2);
}

/**
 * Comparison function.
 * @param source_left
 * @param source_right
 * @return True if source_left < source_right
 */
bool ListDataSortProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
	// Columns must be identical.
	if (source_left.column() != source_right.column()) {
		// Columns don't match. Use standard sorting.
		return super::lessThan(source_left, source_right);
	}

	// Check the sorting method.
	bool bRet;
	switch ((m_sortingMethods >> (source_left.column() * RomFields::COLSORT_BITS)) & RomFields::COLSORT_MASK) {
		default:
			// Unsupported. We'll use standard sorting.
			assert(!"Unsupported sorting method.");
			// fall-through
		case RomFields::COLSORT_STANDARD:
			// Standard sorting.
			bRet = super::lessThan(source_left, source_right);
			break;
		case RomFields::COLSORT_NOCASE: {
			// Case-insensitive sorting.
			QString text1 = source_left.data().toString();
			QString text2 = source_right.data().toString();
			bRet = (QString::compare(text1, text2, Qt::CaseInsensitive) < 0);
			break;
		}
		case RomFields::COLSORT_NUMERIC: {
			// Numeric sorting.
			QString text1 = source_left.data().toString();
			QString text2 = source_right.data().toString();
			bRet = doNumericCompare(text1, text2);
			break;
		}
	}
	return bRet;
}
