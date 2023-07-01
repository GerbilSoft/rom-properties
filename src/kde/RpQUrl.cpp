/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQUrl.cpp: QUrl utility functions                                      *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpQUrl.hpp"

// Qt includes
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#  include <QtCore/QStandardPaths>
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
#  include <QtGui/QDesktopServices>
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

/**
 * Localize a QUrl.
 * This function automatically converts certain URL schemes, e.g. desktop:/, to local paths.
 *
 * @param qUrl QUrl.
 * @return Localized QUrl, or empty QUrl on error.
 */
QUrl localizeQUrl(const QUrl &url)
{
	// Some things work better with local paths than with remote.
	// KDE uses some custom URL schemes, e.g. desktop:/, to represent
	// files that are actually stored locally. Detect this and convert
	// it to a file:/ URL instead.

	// NOTE: KDE's KFileItem has a function to do this, but it only works
	// if KIO::UDSEntry::UDS_LOCAL_PATH is set. This is the case with
	// KPropertiesDialogPlugin, but not the various forwarding plugins
	// when converting a URL from a string.

	// References:
	// - https://bugs.kde.org/show_bug.cgi?id=392100
	// - https://cgit.kde.org/kio.git/commit/?id=7d6e4965dfcd7fc12e8cba7b1506dde22de5d2dd
	// TODO: https://cgit.kde.org/kdenetwork-filesharing.git/commit/?id=abf945afd4f08d80cdc53c650d80d300f245a73d
	// (and other uses) [use mostLocalPath()]

	// TODO: Handle "trash:/"; check KDE for other "local" URL schemes.

	if (url.isEmpty()) {
		// Empty URL. Nothing to do here.
		return url;
	}

	if (url.scheme().isEmpty()) {
		// No scheme. Assume this is a plain old filename.
		const QFileInfo fi(url.path());
		return QUrl::fromLocalFile(fi.absoluteFilePath());
	} else if (url.isLocalFile()) {
		// This is a local file. ("file://" scheme)
		const QFileInfo fi(url.toLocalFile());
		return QUrl::fromLocalFile(fi.absoluteFilePath());
	} else if (url.scheme() == QLatin1String("desktop")) {
		// Desktop folder.
		QString url_path = url.path();
		if (!url_path.isEmpty() && url_path.at(0) == QChar(L'/')) {
			url_path.remove(0, 1);
		}
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		const QString qs_local_filename = QStandardPaths::locate(QStandardPaths::DesktopLocation, url_path);
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
		QString qs_local_filename = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
		if (!qs_local_filename.isEmpty()) {
			if (qs_local_filename.at(qs_local_filename.size()-1) != QChar(L'/')) {
				qs_local_filename += QChar(L'/');
			}
			qs_local_filename += url_path;
		}
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
		return QUrl::fromLocalFile(qs_local_filename);
	}

	// Not a recognized local file scheme.
	// This is probably a remote file.
	return url;
}
