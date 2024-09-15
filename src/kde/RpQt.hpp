/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQt.hpp: Qt wrappers for some libromdata functionality.                *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Qt includes
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtGui/QImage>

// RomPropertiesKDE namespace info
#include "RpQtNS.hpp"

// librptexture
#include "librptexture/img/rp_image.hpp"

#define CONCAT_FN(fn, suffix)		CONCAT_FN_INT(fn, suffix)
#define CONCAT_FN_INT(fn, suffix)	fn ## suffix

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

/**
 * Get const char* from QString.
 * NOTE: This is temporary; assign to an std::string immediately.
 * @param qs QString
 * @return const char*
 */
#define Q2U8(qs) ((qs).toUtf8().constData())

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
		const ushort chr = static_cast<ushort>(lc >> 24);
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
	for (QObject *child : obj->children()) {
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
 * @param image rp_image
 * @return QImage.
 */
QImage rpToQImage(const LibRpTexture::rp_image *image);

/**
 * Convert an rp_image to QImage.
 * @param image rp_image_ptr
 * @return QImage.
 */
static inline QImage rpToQImage(const LibRpTexture::rp_image_ptr &image)
{
	return rpToQImage(image.get());
}

/**
 * Convert an rp_image to QImage.
 * @param image rp_image_const_ptr
 * @return QImage.
 */
static inline QImage rpToQImage(const LibRpTexture::rp_image_const_ptr &image)
{
	return rpToQImage(image.get());
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
QString rpFileDialogFilterToQt(const char *filter);
