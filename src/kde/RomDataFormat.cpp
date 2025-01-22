/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomDataFormat.hpp: Common RomData string formatting functions.          *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RomDataFormat.hpp"

// librpbase
#include "librpbase/RomFields.hpp"
using LibRpBase::RomFields;

// C++ STL classes
using std::string;

/**
 * Format an RFT_DATETIME.
 * @param date_time	[in] Date/Time
 * @param flags		[in] RFT_DATETIME flags
 * @return Formatted RFT_DATETIME, or empty on error.
 */
QString formatDateTime(time_t date_time, unsigned int flags)
{
	const QDateTime dateTime = unixTimeToQDateTime(date_time, !!(flags & RomFields::RFT_DATETIME_IS_UTC));

	QString str;
	const QLocale locale = QLocale::system();
	switch (flags & RomFields::RFT_DATETIME_HAS_DATETIME_NO_YEAR_MASK) {
		case RomFields::RFT_DATETIME_HAS_DATE:
			// Date only.
			str = locale.toString(dateTime.date(), locale.dateFormat(QLocale::ShortFormat));
			break;

		case RomFields::RFT_DATETIME_HAS_TIME:
		case RomFields::RFT_DATETIME_HAS_TIME |
		     RomFields::RFT_DATETIME_NO_YEAR:
			// Time only.
			str = locale.toString(dateTime.time(), locale.timeFormat(QLocale::ShortFormat));
			break;

		case RomFields::RFT_DATETIME_HAS_DATE |
		     RomFields::RFT_DATETIME_HAS_TIME:
			// Date and time.
			str = locale.toString(dateTime, locale.dateTimeFormat(QLocale::ShortFormat));
			break;

		case RomFields::RFT_DATETIME_HAS_DATE |
		     RomFields::RFT_DATETIME_NO_YEAR:
			// Date only. (No year)
			// TODO: Localize this.
			str = locale.toString(dateTime.date(), QLatin1String("MMM d"));
			break;

		case RomFields::RFT_DATETIME_HAS_DATE |
		     RomFields::RFT_DATETIME_HAS_TIME |
		     RomFields::RFT_DATETIME_NO_YEAR:
			// Date and time. (No year)
			// TODO: Localize this.
			str = dateTime.date().toString(QLatin1String("MMM d")) + QChar(L' ') +
			      dateTime.time().toString();
			break;

		default:
			// Invalid combination.
			assert(!"Invalid Date/Time formatting.");
			break;
	}

	return str;
}

/**
 * Format an RFT_DIMENSIONS.
 * @param dimensions	[in] Dimensions
 * @return Formatted RFT_DIMENSIONS, or nullptr on error.
 */
QString formatDimensions(const int dimensions[3])
{
	// TODO: 'x' or '×'? Using 'x' for now.
	string str;

	if (dimensions[1] > 0) {
		if (dimensions[2] > 0) {
			str = fmt::format(FSTR("{:d}x{:d}x{:d}"),
				dimensions[0], dimensions[1], dimensions[2]);
		} else {
			str = fmt::format(FSTR("{:d}x{:d}"),
				dimensions[0], dimensions[1]);
		}
	} else {
		str = fmt::to_string(dimensions[0]);
	}

	return QString::fromLatin1(str.c_str());
}
