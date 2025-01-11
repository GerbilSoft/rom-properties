/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomDataFormat.hpp: Common RomData string formatting functions.          *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RomDataFormat.hpp"

// librpbase
#include "librpbase/RomFields.hpp"
using LibRpBase::RomFields;

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
	char buf[64];
	if (dimensions[1] > 0) {
		if (dimensions[2] > 0) {
			snprintf(buf, sizeof(buf), "%dx%dx%d",
				dimensions[0], dimensions[1], dimensions[2]);
		} else {
			snprintf(buf, sizeof(buf), "%dx%d",
				dimensions[0], dimensions[1]);
		}
	} else {
		snprintf(buf, sizeof(buf), "%d", dimensions[0]);
	}

	return QString::fromLatin1(buf);
}
