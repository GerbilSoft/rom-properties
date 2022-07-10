/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQt.cpp: Qt wrappers for some libromdata functionality.                *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpQt.hpp"
#include "RpQImageBackend.hpp"

// librpbase, librpfile, librptexture
#include "librpbase/config/Config.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using LibRpTexture::rp_image;

// RpFileKio
#include "RpFile_kio.hpp"

// C++ STL classes
using std::u8string;

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#  include <QtCore/QStandardPaths>
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
#  include <QtGui/QDesktopServices>
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

/** Image conversion **/

/**
 * Convert an rp_image to QImage.
 * @param image rp_image.
 * @return QImage.
 */
QImage rpToQImage(const rp_image *image)
{
	if (!image || !image->isValid())
		return QImage();

	// We should be using the RpQImageBackend.
	const RpQImageBackend *backend =
		dynamic_cast<const RpQImageBackend*>(image->backend());
	assert(backend != nullptr);
	if (!backend) {
		// Incorrect backend set.
		return QImage();
	}

	return backend->getQImage();
}

/** QUrl **/

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
		QFileInfo fi(url.path());
		return QUrl::fromLocalFile(fi.absoluteFilePath());
	} else if (url.isLocalFile()) {
		// This is a local file. ("file://" scheme)
		QFileInfo fi(url.toLocalFile());
		return QUrl::fromLocalFile(fi.absoluteFilePath());
	} else if (url.scheme() == QLatin1String("desktop")) {
		// Desktop folder.
		QString url_path = url.path();
		if (!url_path.isEmpty() && url_path.at(0) == QChar(L'/')) {
			url_path.remove(0, 1);
		}
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		QString qs_local_filename = QStandardPaths::locate(QStandardPaths::DesktopLocation, url_path);
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

/**
 * Open a QUrl as an IRpFile. (read-only)
 * This function automatically converts certain URL schemes, e.g. desktop:/, to local paths.
 *
 * @param qUrl QUrl.
 * @param isThumbnail If true, this file is being used for thumbnailing. Handle "bad FS" checking.
 *
 * @return IRpFile, or nullptr on error.
 */
IRpFile *openQUrl(const QUrl &url, bool isThumbnail)
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
		return nullptr;
	}

	QUrl localUrl = localizeQUrl(url);
	if (localUrl.isEmpty()) {
		// Unable to localize the URL.
		return nullptr;
	}

	u8string s_local_filename;
	if (localUrl.scheme().isEmpty() || localUrl.isLocalFile()) {
		s_local_filename = reinterpret_cast<const char8_t*>(
			localUrl.toLocalFile().toUtf8().constData());
	}

	if (isThumbnail) {
		// We're thumbnailing the file. Check "bad FS" settings.
		const Config *const config = Config::instance();
		const bool enableThumbnailOnNetworkFS = config->enableThumbnailOnNetworkFS();
		if (!s_local_filename.empty()) {
			// This is a local file. Check if it's on a "bad" file system.
			// FIXME: U8STRFIX
			if (FileSystem::isOnBadFS(reinterpret_cast<const char*>(s_local_filename.c_str()), enableThumbnailOnNetworkFS)) {
				// This file is on a "bad" file system.
				return nullptr;
			}
		} else {
			// This is a remote file. Assume it's a "bad" file system.
			if (!enableThumbnailOnNetworkFS) {
				// Thumbnailing on network file systems is disabled.
				return nullptr;
			}
		}
	}

	// Attempt to open an IRpFile.
	IRpFile *file;
	if (!s_local_filename.empty()) {
		// Local filename. Use RpFile.
		file = new RpFile(s_local_filename, RpFile::FM_OPEN_READ_GZ);
	} else {
		// Remote filename. Use RpFile_kio.
#ifdef HAVE_RPFILE_KIO
		file = new RpFileKio(url);
#else /* !HAVE_RPFILE_KIO */
		// Not supported...
		return nullptr;
#endif
	}

	if (!file->isOpen()) {
		// Unable to open the file...
		// TODO: Return an error code?
		UNREF_AND_NULL_NOCHK(file);
	}

	return file;
}

/**
 * Convert an RP file dialog filter to Qt.
 *
 * RP syntax: "Sega Mega Drive ROM images|*.gen;*.bin|application/x-genesis-rom|All Files|*.*|-"
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

	// Temporary string so we can use strtok_r().
	char *const tmpfilter = strdup(filter);
	assert(tmpfilter != nullptr);
	char *saveptr = nullptr;

	// First strtok_r() call.
	qs_ret.reserve(strlen(filter) + 32);
	char *token = strtok_r(tmpfilter, "|", &saveptr);
	do {
		// Separator 1: Between display name and pattern.
		// (strtok_r() call was done in the previous iteration.)
		assert(token != nullptr);
		if (!token) {
			// Missing token...
			free(tmpfilter);
			return QString();
		}
		if (!qs_ret.isEmpty()) {
			qs_ret += QLatin1String(";;");
		}
		qs_ret += QString::fromUtf8(token);

		// Separator 2: Between pattern and MIME types.
		token = strtok_r(nullptr, "|", &saveptr);
		assert(token != nullptr);
		if (!token) {
			// Missing token...
			free(tmpfilter);
			return QString();
		}

		QString patterns = QString::fromUtf8(token);
		patterns.replace(QChar(L';'), QChar(L' '));
		qs_ret += QLatin1String(" (");
		qs_ret += patterns;
		qs_ret += QChar(L')');

		// Separator 3: Between MIME types and the next display name.
		// NOTE: May be missing if this is the end of the string
		// and a MIME type isn't set.
		// NOTE: Not used by Qt.
		token = strtok_r(nullptr, "|", &saveptr);

		// Next token.
		token = strtok_r(nullptr, "|", &saveptr);
	} while (token != nullptr);

	free(tmpfilter);
	return qs_ret;
}
