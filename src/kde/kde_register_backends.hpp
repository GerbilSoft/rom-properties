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

#if !defined(RP_KDE_DISABLE_REGISTER_ACHQTDBUS) && defined(ENABLE_ACHIEVEMENTS) && defined(HAVE_QtDBus_NOTIFY)
// Achievements backend
#include "AchQtDBus.hpp"
#endif /* !RP_KDE_DISABLE_REGISTER_ACHQTDBUS && ENABLE_ACHIEVEMENTS && HAVE_QtDBus_NOTIFY */

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
// LibRpText for Binary Unit BinaryUnitDialect
#  include "librptext/formatting.hpp"
// Qt
#  include <QtCore/QSettings>
#  include <QtCore/QStandardPaths>

/**
 * User setting query function for LibRpText.
 * @param user_data	[in] User data from registerNotifyFunction()
 * @param setting	[in] User setting to retrieve
 * @return User setting on success; -1 on error.
 */
static int getBinaryUnitDialect(void *user_data, LibRpText::UserSetting setting)
{
	RP_UNUSED(user_data);
	if (setting != LibRpText::UserSetting::BinaryUnitDialect) {
		// Not a supported setting value.
		return -1;
	}

	// from KFormatPrivate::formatByteSize()
	// NOTE: KFormat::BinaryUnitDialect maps directly to LibRpText::BinaryUnitDialect.
	static constexpr LibRpText::BinaryUnitDialect fallbackDialect = LibRpText::BinaryUnitDialect::IECBinaryDialect;

	const QString kdeglobals = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QLatin1String("kdeglobals"));
	QSettings settings(kdeglobals, QSettings::IniFormat);
	return settings.value("Locale/BinaryUnitDialect", static_cast<int>(fallbackDialect)).toInt();
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
