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

#include "libromdata/config.libromdata.h"

namespace LibRomData {
	class rp_image;
}

// Qt includes.
#include <QtCore/QString>
#include <QtGui/QImage>

#if !defined(RP_UTF8) && !defined(RP_UTF16)
#error Neither RP_UTF8 nor RP_UTF16 has been defined.
#elif defined(RP_UTF8) && defined(RP_UTF16)
#error Both RP_UTF8 and RP_UTF16 are defined, but only one should be.
#endif

/** Text conversion. **/

// NOTE: QChar contains a single field, a ushort ucs value.
// Hence, we can cast UTF-16 strings to QChar* and use the
// QString(const QChar*) constructor instead of fromUtf16(),
// which is "comparatively slow" due to BOM checking.

/**
 * NOTE: Some of the UTF-8 versions return toUtf8().constData()
 * from the QString. Therefore, you *must* assign the
 * the result to an rp_string if storing it, since an
 * rp_char* will result in a dangling pointer.
 */

#ifdef RP_UTF8

/**
 * Convert an rp_string to a QString.
 * @param rps rp_string
 * @return QString
 */
static inline QString RP2Q(const LibRomData::rp_string &rps)
{
	return QString::fromUtf8(rps.data(), (int)rps.size());
}

/**
 * Convert a const rp_char* to a QString.
 * @param str const rp_char*
 * @param len Length of str, in characters. (optional; -1 for C string)
 * @return QString
 */
static inline QString RP2Q(const rp_char *rps, int len = -1)
{
	return QString::fromUtf8(rps, len);
}

/**
 * Get const rp_char* from QString.
 * @param qs QString
 * @return const rp_char*
 */
#define Q2RP(qs) (reinterpret_cast<const rp_char*>(((qs).toUtf8().constData())))

#elif defined(RP_UTF16)

/**
 * Convert an rp_string to a QString.
 * @param rps rp_string
 * @return QString
 */
static inline QString RP2Q(const LibRomData::rp_string &rps)
{
	return QString(reinterpret_cast<const QChar*>(rps.data()), (int)rps.size());
}

/**
 * Convert a const rp_char* to a QString.
 * @param str const rp_char*
 * @param len Length of str, in characters. (optional; -1 for C string)
 * @return QString
 */
static inline QString RP2Q(const rp_char *rps, int len = -1)
{
	return QString(reinterpret_cast<const QChar*>(rps), len);
}

/**
 * Get const rp_char* from QString.
 * @param qs QString
 * @return const rp_char*
 */
static inline const rp_char *Q2RP(const QString &qs)
{
	return reinterpret_cast<const rp_char*>(qs.utf16());
}

#endif

/** Image conversion. **/

/**
 * Convert an rp_image to QImage.
 * TODO: Move to another file?
 * @param image rp_image.
 * @return QImage.
 */
QImage rpToQImage(const LibRomData::rp_image *image);

#endif /* __ROMPROPERTIES_KDE_RPQT_HPP__ */
