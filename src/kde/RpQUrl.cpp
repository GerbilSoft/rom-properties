/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQUrl.cpp: QUrl utility functions                                      *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpQUrl.hpp"

// RpFileKio
#include "RpFile_kio.hpp"

// Other rom-properties libraries
#include "librpbase/config/Config.hpp"
using LibRpBase::Config;
using namespace LibRpFile;

// Qt includes
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#  include <QtCore/QStandardPaths>
#else /* QT_VERSION < QT_VERSION_CHECK(5, 0, 0) */
#  include <QtGui/QDesktopServices>
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) */

// C++ STL classes
using std::string;

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
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		const QString qs_local_filename = QStandardPaths::locate(QStandardPaths::DesktopLocation, url_path);
#else /* QT_VERSION < QT_VERSION_CHECK(5, 0, 0) */
		QString qs_local_filename = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
		if (!qs_local_filename.isEmpty()) {
			if (qs_local_filename.at(qs_local_filename.size()-1) != QChar(L'/')) {
				qs_local_filename += QChar(L'/');
			}
			qs_local_filename += url_path;
		}
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) */
		return QUrl::fromLocalFile(qs_local_filename);
	}

	// Not a recognized local file scheme.
	// This is probably a remote file.
	return url;
}

/**
 * Open a QUrl as an IRpFile. (read-only)
 * This function automatically converts certain URL schemes, e.g. desktop:/, to local paths.
 *
 * @param qUrl QUrl.
 * @param isThumbnail If true, this file is being used for thumbnailing. Handle "bad FS" checking.
 *
 * @return IRpFile, or nullptr on error.
 */
IRpFilePtr openQUrl(const QUrl &url, bool isThumbnail)
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

	// NOTE: "trash:/" isn't handled, but Dolphin 23.08.3 attempts to
	// thumbnail both the actual local filename in ~/.local/share/Trash/
	// *and* the trash:/ URL, so it doesn't matter.
	// TODO: Check KDE for other "local" URL schemes.

	if (url.isEmpty()) {
		// Empty URL. Nothing to do here.
		return {};
	}

	const QUrl localUrl = localizeQUrl(url);
	if (localUrl.isEmpty()) {
		// Unable to localize the URL.
		return {};
	}

	string s_local_filename;
	if (localUrl.scheme().isEmpty()) {
		s_local_filename = localUrl.path().toUtf8().constData();
	} else if (localUrl.isLocalFile()) {
		s_local_filename = localUrl.toLocalFile().toUtf8().constData();
	}

	if (isThumbnail) {
		// We're thumbnailing the file. Check "bad FS" settings.
		const Config *const config = Config::instance();
		const bool enableThumbnailOnNetworkFS = config->getBoolConfigOption(Config::BoolConfig::Options_EnableThumbnailOnNetworkFS);
		if (!s_local_filename.empty()) {
			// This is a local file. Check if it's on a "bad" file system.
			if (FileSystem::isOnBadFS(s_local_filename, enableThumbnailOnNetworkFS)) {
				// This file is on a "bad" file system.
				return {};
			}
		} else {
			// This is a remote file. Assume it's a "bad" file system.
			if (!enableThumbnailOnNetworkFS) {
				// Thumbnailing on network file systems is disabled.
				return {};
			}
		}
	}

	// Attempt to open an IRpFile.
	IRpFilePtr file;
	if (!s_local_filename.empty()) {
		// Local filename. Use RpFile.
		file = std::make_shared<RpFile>(s_local_filename, RpFile::FM_OPEN_READ_GZ);
	} else {
		// Remote filename. Use RpFile_kio.
#ifdef HAVE_RPFILE_KIO
		file = std::make_shared<RpFileKio>(url);
#else /* !HAVE_RPFILE_KIO */
		// Not supported...
		return {};
#endif
	}

	if (file && file->isOpen()) {
		// File opened successfully.
		return file;
	}

	// Unable to open the file...
	// TODO: Return an error code?
	return {};
}
