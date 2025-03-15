/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomThumbCreator_p.cpp: Thumbnail creator. (PRIVATE CLASS)               *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.kde.h"

#include "RomThumbCreator_p.hpp"

#if defined(ENABLE_NETWORKING) && QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) && defined(HAVE_QtDBus)
// NetworkManager D-Bus interface to determine if the connection is metered.
// FIXME: Broken on Qt4.
#  define ENABLE_METER_CHECK 1
#endif /* defined(ENABLE_NETWORKING) && QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) && defined(HAVE_QtDBus) */

#ifdef ENABLE_METER_CHECK
// NetworkManager D-Bus interface to determine if the connection is metered.
#  include "networkmanagerinterface.h"
#endif /* ENABLE_METER_CHECK */

/** TCreateThumbnail functions **/

/**
 * Rescale an ImgClass using the specified scaling method.
 * @param imgClass ImgClass object.
 * @param sz New size.
 * @param method Scaling method.
 * @return Rescaled ImgClass.
 */
QImage RomThumbCreatorPrivate::rescaleImgClass(const QImage &imgClass, ImgSize sz, ScalingMethod method) const
{
	Qt::TransformationMode mode;
	switch (method) {
		case ScalingMethod::Nearest:
			mode = Qt::FastTransformation;
			break;
		case ScalingMethod::Bilinear:
			mode = Qt::SmoothTransformation;
			break;
		default:
			assert(!"Invalid scaling method.");
			mode = Qt::FastTransformation;
			break;
	}

	QImage img = imgClass.scaled(sz.width, sz.height, Qt::IgnoreAspectRatio, mode);

	// NOTE: Rescaling an ARGB32 image sometimes results in the format
	// being changes to QImage::Format_ARGB32_Premultiplied.
	// Convert it back to plain ARGB32 if that happens.
	if (img.format() == QImage::Format_ARGB32_Premultiplied) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
		img.convertTo(QImage::Format_ARGB32);
#else /* QT_VERSION < QT_VERSION_CHECK(5, 13, 0) */
		img = img.convertToFormat(QImage::Format_ARGB32);
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 13, 0) */
	}

	return img;
}

/**
 * Is the system using a metered connection?
 *
 * Note that if the system doesn't support identifying if the
 * connection is metered, it will be assumed that the network
 * connection is unmetered.
 *
 * @return True if metered; false if not.
 */
bool RomThumbCreatorPrivate::isMetered(void)
{
#ifdef ENABLE_METER_CHECK
	org::freedesktop::NetworkManager iface(
		QLatin1String("org.freedesktop.NetworkManager"),
		QLatin1String("/org/freedesktop/NetworkManager"),
		QDBusConnection::systemBus());
	if (!iface.isValid()) {
		// Invalid interface.
		// Assume unmetered.
		return false;
	}

	// https://developer-old.gnome.org/NetworkManager/stable/nm-dbus-types.html#NMMetered
	enum NMMetered : uint32_t {
		NM_METERED_UNKNOWN	= 0,
		NM_METERED_YES		= 1,
		NM_METERED_NO		= 2,
		NM_METERED_GUESS_YES	= 3,
		NM_METERED_GUESS_NO	= 4,
	};
	const NMMetered metered = static_cast<NMMetered>(iface.metered());
	return (metered == NM_METERED_YES || metered == NM_METERED_GUESS_YES);
#else /* !ENABLE_METER_CHECK */
	// FIXME: Broken on Qt4.
	return false;
#endif /* ENABLE_METER_CHECK */
}
