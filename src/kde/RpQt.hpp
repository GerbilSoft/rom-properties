/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQt.hpp: Qt wrappers for some libromdata functionality.                *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_RPQT_HPP__
#define __ROMPROPERTIES_KDE_RPQT_HPP__

namespace LibRpTexture {
	class rp_image;
}

// C++ includes.
#include <string>

// Qt includes.
#include <QtCore/QString>
#include <QtGui/QImage>

// KDE Frameworks prefix. (KDE4/KF5)
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
# define RP_KDE_UPPER "KF"
# define RP_KDE_LOWER "kf"
#else /* !QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
# define RP_KDE_UPPER "KDE"
# define RP_KDE_LOWER "kde"
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

/** Text conversion. **/

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
	return QString::fromUtf8(str.data(), (int)str.size());
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
#define Q2U8(qs) (reinterpret_cast<const char*>(((qs).toUtf8().constData())))

/** Image conversion. **/

/**
 * Convert an rp_image to QImage.
 * @param image rp_image.
 * @return QImage.
 */
QImage rpToQImage(const LibRpTexture::rp_image *image);

#endif /* __ROMPROPERTIES_KDE_RPQT_HPP__ */
