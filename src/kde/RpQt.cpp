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
#include "librpbase/config/Config.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpTexture;

// RpFileKio
#include "RpFile_kio.hpp"

// C++ STL classes
using std::string;

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

/** QUrl **/

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
		return nullptr;
	}

	const QUrl localUrl = localizeQUrl(url);
	if (localUrl.isEmpty()) {
		// Unable to localize the URL.
		return nullptr;
	}

	string s_local_filename;
	if (localUrl.scheme().isEmpty() || localUrl.isLocalFile()) {
		s_local_filename = localUrl.toLocalFile().toUtf8().constData();
	}

	if (isThumbnail) {
		// We're thumbnailing the file. Check "bad FS" settings.
		const Config *const config = Config::instance();
		const bool enableThumbnailOnNetworkFS = config->enableThumbnailOnNetworkFS();
		if (!s_local_filename.empty()) {
			// This is a local file. Check if it's on a "bad" file system.
			if (FileSystem::isOnBadFS(s_local_filename.c_str(), enableThumbnailOnNetworkFS)) {
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
		return nullptr;
#endif
	}

	if (file && file->isOpen()) {
		// File opened successfully.
		return file;
	}

	// Unable to open the file...
	// TODO: Return an error code?
	return nullptr;
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
