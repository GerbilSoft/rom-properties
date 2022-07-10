/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQt.hpp: Qt wrappers for some libromdata functionality.                *
 *                                                                         *
 * Copyright (c) 2016-202 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_RPQT_HPP__
#define __ROMPROPERTIES_KDE_RPQT_HPP__

namespace LibRpFile {
	class IRpFile;
}
namespace LibRpTexture {
	class rp_image;
}

// C++ includes.
#include <string>

// Qt includes
#include <qglobal.h>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtGui/QImage>

#if QT_VERSION >= QT_VERSION_CHECK(7,0,0)
#  error Needs updating for Qt7
#elif QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#  define RP_KDE_SUFFIX KF6
#  define RP_KDE_UPPER "KF6"
#  define RP_KDE_LOWER "kf6"
#  define RomPropertiesKDE RomPropertiesKF6
#elif QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#  define RP_KDE_SUFFIX KF5
#  define RP_KDE_UPPER "KF5"
#  define RP_KDE_LOWER "kf5"
#  define RomPropertiesKDE RomPropertiesKF5
#elif QT_VERSION >= QT_VERSION_CHECK(4,0,0)
#  define RP_KDE_SUFFIX KDE4
#  define RP_KDE_UPPER "KDE4"
#  define RP_KDE_LOWER "kde4"
#  define RomPropertiesKDE RomPropertiesKDE4
#else /* QT_VERSION < QT_VERSION_CHECK(4,0,0) */
#  error Qt version is too old
#endif

/** Text conversion **/

/**
 * NOTE: Some of the UTF-8 functions return toUtf8().constData()
 * from the QString. Therefore, you *must* assign the result to
 * an std::string if storing it, since storing it as const char*
 * will result in a dangling pointer.
 */

/**
 * Convert an std::string to QString.
 * @param str std::string
 * @return QString
 */
static inline QString U82Q(const std::string &str)
{
	return QString::fromUtf8(str.data(), static_cast<int>(str.size()));
}

/**
 * Convert a const char* to a QString.
 * @param str const char*
 * @param len Length of str, in characters. (optional; -1 for C string)
 * @return QString
 */
static inline QString U82Q(const char *str, int len = -1)
{
	return QString::fromUtf8(str, len);
}

#ifndef CXX20_COMPAT_CHAR8_T
/**
 * Convert an std::u8string to QString.
 * @param str std::u8string
 * @return QString
 */
static inline QString U82Q(const std::u8string &str)
{
	return QString::fromUtf8(reinterpret_cast<const char*>(str.data()), static_cast<int>(str.size()));
}

/**
 * Convert a const char* to a QString.
 * @param str const char*
 * @param len Length of str, in characters. (optional; -1 for C string)
 * @return QString
 */
static inline QString U82Q(const char8_t *str, int len = -1)
{
	return QString::fromUtf8(reinterpret_cast<const char*>(str), len);
}
#endif /* CXX20_COMPAT_CHAR8_T */

/**
 * Get const char8_t* from QString.
 * NOTE: This is temporary; assign to an std::u8string immediately.
 * @param qs QString
 * @return const char8_t*
 */
#define Q2U8(qs) (reinterpret_cast<const char8_t*>((qs).toUtf8().constData()))

/**
 * Convert a language code to a QString.
 * @param lc Language code.
 * @return QString.
 */
static inline QString lcToQString(uint32_t lc)
{
	QString s_lc;
	s_lc.reserve(4);
	for (; lc != 0; lc <<= 8) {
		ushort chr = (ushort)(lc >> 24);
		if (chr != 0) {
			s_lc += QChar(chr);
		}
	}
	return s_lc;
}

/** QObject **/

/**
 * Find direct child widgets only.
 * @param T Type.
 * @param aName Name to match, or empty string for any object of type T.
 */
template<typename T>
static inline T findDirectChild(QObject *obj, const QString &aName = QString())
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	return obj->findChild<T>(aName, Qt::FindDirectChildrenOnly);
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
	foreach(QObject *child, obj->children()) {
		T qchild = qobject_cast<T>(child);
		if (qchild != nullptr) {
			if (aName.isEmpty() || qchild->objectName() == aName) {
				return qchild;
			}
		}
	}
	return nullptr;
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
}

/** Image conversion **/

/**
 * Convert an rp_image to QImage.
 * @param image rp_image.
 * @return QImage.
 */
QImage rpToQImage(const LibRpTexture::rp_image *image);

/** QUrl **/

/**
 * Localize a QUrl.
 * This function automatically converts certain URL schemes, e.g. desktop:/, to local paths.
 *
 * @param qUrl QUrl.
 * @return Localized QUrl, or empty QUrl on error.
 */
QUrl localizeQUrl(const QUrl &url);

/**
 * Open a QUrl as an IRpFile. (read-only)
 * This function automatically converts certain URL schemes, e.g. desktop:/, to local paths.
 *
 * @param qUrl QUrl.
 * @param isThumbnail If true, this file is being used for thumbnailing. Handle "bad FS" checking.
 *
 * @return IRpFile, or nullptr on error.
 */
LibRpFile::IRpFile *openQUrl(const QUrl &url, bool isThumbnail = false);

/**
 * Convert an RP file dialog filter to Qt.
 *
 * RP syntax: "Sega Mega Drive ROM images|*.gen;*.bin|application/x-genesis-rom|All Files|*.*|-"
 * Similar the same as Windows, but with '|' instead of '\0'.
 * Also, no terminator sequence is needed.
 * The "(*.bin; *.srl)" part is added to the display name if needed.
 * A third segment provides for semicolon-separated MIME types. (May be "-" for 'any'.)
 *
 * @param filter RP file dialog filter (UTF-8, from gettext())
 * @return Qt file dialog filter
 */
QString rpFileDialogFilterToQt(const char8_t *filter);

#endif /* __ROMPROPERTIES_KDE_RPQT_HPP__ */
