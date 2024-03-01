/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQt.cpp: Qt wrappers for some libromdata functionality.                *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpQt.hpp"
#include "RpQImageBackend.hpp"

// Other rom-properties libraries
using LibRpTexture::rp_image;

/** Image conversion **/

/**
 * Convert an rp_image to QImage.
 * @param image rp_image.
 * @return QImage.
 */
QImage rpToQImage(const rp_image *image)
{
	if (!image || !image->isValid())
		return {};

	// We should be using the RpQImageBackend.
	const RpQImageBackend *backend =
		dynamic_cast<const RpQImageBackend*>(image->backend());
	assert(backend != nullptr);
	if (!backend) {
		// Incorrect backend set.
		return {};
	}

	return backend->getQImage();
}

/**
 * Convert an RP file dialog filter to Qt.
 *
 * RP syntax: "Sega Mega Drive ROM images|*.gen;*.bin|application/x-genesis-rom|All Files|*|-"
 * Similar the same as Windows, but with '|' instead of '\0'.
 * Also, no terminator sequence is needed.
 * The "(*.bin; *.srl)" part is added to the display name if needed.
 * A third segment provides for semicolon-separated MIME types. (May be "-" for 'any'.)
 *
 * @param filter RP file dialog filter. (UTF-8, from gettext())
 * @return Qt file dialog filter.
 */
QString rpFileDialogFilterToQt(const char *filter)
{
	QString qs_ret;
	assert(filter != nullptr && filter[0] != '\0');
	if (!filter || filter[0] == '\0')
		return qs_ret;

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
#  define Qt_KeepEmptyParts Qt::KeepEmptyParts
#else /* QT_VERSION < QT_VERSION_CHECK(5,14,0) */
#  define Qt_KeepEmptyParts QString::KeepEmptyParts
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,14,0) */

	// Using QString::split() instead of strtok_r() so we don't
	// have to manually strdup() the filter.
	const QString qs_filter = QString::fromUtf8(filter);
	QStringList sl = qs_filter.split(QChar(L'|'), Qt_KeepEmptyParts);
	assert(sl.size() % 3 == 0);
	if (sl.size() % 3 != 0) {
		// Not a multiple of 3.
		return qs_ret;
	}

	qs_ret.reserve(qs_filter.size() + (sl.size() * 5));
	for (int i = 0; i < sl.size(); i += 3) {
		// String indexes:
		// - 0: Display name
		// - 1: Pattern
		// - 2: MIME type (optional; not used by Qt)
		if (!qs_ret.isEmpty()) {
			qs_ret += QLatin1String(";;");
		}
		qs_ret += sl[i+0];
		qs_ret += QLatin1String(" (");
		qs_ret += sl[i+1];
		qs_ret += QChar(L')');
	}

	return qs_ret;
}
