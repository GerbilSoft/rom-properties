/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * ListDataSortProxyModel.cpp: QSortFilterProxyModel for RFT_LISTDATA.     *
 *                                                                         *
 * Copyright (c) 2012-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
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

	const bool isAllNumeric = (pos == str.size());
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
 * @param strA
 * @param strB
 * @return True if strA < strB
 */
bool ListDataSortProxyModel::doNumericCompare(const QString &strA, const QString &strB)
{
	if (strA.isEmpty() && strB.isEmpty()) {
		// Both strings are empty.
		return false;
	}

	bool okA = false, okB = false;
	const qlonglong valA = parseQString(strA, &okA);
	const qlonglong valB = parseQString(strB, &okB);

	if (valA == valB) {
		// Values are identical.
		if (okA && !okB) {
			// Second string is not fully numeric.
			// strA < strB
			return true;
		} else {
			// Other cases: strA >= strB
			return false;
		}
	}

	return (valA < valB);
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
			const QString strA = source_left.data().toString();
			const QString strB = source_right.data().toString();
			bRet = (QString::compare(strA, strB, Qt::CaseInsensitive) < 0);
			break;
		}
		case RomFields::COLSORT_NUMERIC: {
			// Numeric sorting.
			const QString strA = source_left.data().toString();
			const QString strB = source_right.data().toString();
			bRet = doNumericCompare(strA, strB);
			break;
		}
	}
	return bRet;
}
