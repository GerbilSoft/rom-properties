/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RpQt.hpp: Qt wrappers for some libromdata functionality.                *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_RPQT_HPP__
#define __ROMPROPERTIES_KDE_RPQT_HPP__

#include <string>

namespace LibRpBase {
	class rp_image;
}

// Qt includes.
#include <QtCore/QString>
#include <QtGui/QImage>

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
QImage rpToQImage(const LibRpBase::rp_image *image);

#endif /* __ROMPROPERTIES_KDE_RPQT_HPP__ */
