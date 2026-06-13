/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * kde_register_backends.hpp: Register Backends function.                  *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.kde.h"

// Qt includes
#include <QtCore/qglobal.h>

// RpQImageBackend
#include "RpQImageBackend.hpp"
using LibRpTexture::rp_image;

#ifndef RP_KDE_DISABLE_REGISTER_ACHQTDBUS
// Achievements backend
#include "AchQtDBus.hpp"
#endif /* !RP_KDE_DISABLE_REGISTER_ACHQTDBUS */

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
// LibRpText for Binary Unit BinaryUnitDialect
#  include "librptext/formatting.hpp"
// Qt
#  include <QtCore/QSettings>
#  include <QtCore/QStandardPaths>

/**
 * Get the BinaryUnitDialect from KDE Globals.
 * Added in KCoreAddons v5.245.0.
 * @param user_data
 * @return BinaryUnitDialect
 */
static LibRpText::BinaryUnitDialect getBinaryUnitDialect(void *user_data)
{
	// from KFormatPrivate::formatByteSize()
	// NOTE: KFormat::BinaryUnitDialect maps directly to LibRpText::BinaryUnitDialect.
	static constexpr LibRpText::BinaryUnitDialect fallbackDialect = LibRpText::BinaryUnitDialect::IECBinaryDialect;
	RP_UNUSED(user_data);

	const QString kdeglobals = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QLatin1String("kdeglobals"));
	QSettings settings(kdeglobals, QSettings::IniFormat);
	return static_cast<LibRpText::BinaryUnitDialect>(settings.value("Locale/BinaryUnitDialect", static_cast<int>(fallbackDialect)).toInt());
}
#endif /* QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) */

/**
 * Register the KDE backends for e.g. rp_image.
 * This must be called when a plugin or executable is loaded.
 */
static inline void kde_register_backends(void)
{
	// Register RpQImageBackend for rp_image.
	rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);

#if !defined(RP_KDE_DISABLE_REGISTER_ACHQTDBUS) && defined(ENABLE_ACHIEVEMENTS) && defined(HAVE_QtDBus_NOTIFY)
	// Register AchQtDBus for achievements.
	AchQtDBus::instance();
#endif /* !RP_KDE_DISABLE_REGISTER_ACHQTDBUS && ENABLE_ACHIEVEMENTS && HAVE_QtDBus_NOTIFY */

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	// Register getBinaryUnitDialect() for LibRpText::formatFileSize().
	// Added in KCoreAddons v5.245.0.
	LibRpText::setUserSettingQueryFunction(getBinaryUnitDialect, nullptr);
#endif /* QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) */
}
