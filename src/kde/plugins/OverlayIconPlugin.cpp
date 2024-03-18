/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * OverlayIconPlugin.cpp: KOverlayIconPlugin.                              *
 *                                                                         *
 * NOTE: This file is compiled as a separate .so file. Originally, a       *
 * forwarder plugin was used, since Qt's plugin system prevents a single   *
 * shared library from exporting multiple plugins, but as of RP 2.0,       *
 * most of the important code is split out into libromdata.so, so the      *
 * forwarder version is unnecessary.                                       *
 *                                                                         *
 * Copyright (c) 2018-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "check-uid.hpp"

#include <QtCore/qglobal.h>
#if QT_VERSION >= QT_VERSION_CHECK(7,0,0)
#  error Update for new Qt!
#elif QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#  include "OverlayIconPluginKF6.hpp"
#elif QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#  include "OverlayIconPluginKF5.hpp"
#else
#  error Qt is too old!
#endif

#include "RpQUrl.hpp"

// Other rom-properties libraries
#include "libromdata/RomDataFactory.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRomData;

// C++ STL classes
using std::string;

// Qt includes.
#include <QtCore/QStandardPaths>

namespace RomPropertiesKDE {

OverlayIconPlugin::OverlayIconPlugin(QObject *parent)
	: super(parent)
{}

QStringList OverlayIconPlugin::getOverlays(const QUrl &item)
{
	// TODO: Check for slow devices and/or cache this?
	QStringList sl;

	const Config *const config = Config::instance();
	if (!config->showDangerousPermissionsOverlayIcon()) {
		// Overlay icon is disabled.
		return sl;
	}

	// Attempt to open the ROM file.
	const IRpFilePtr file(openQUrl(item, true));
	if (!file) {
		// Could not open the file.
		return sl;
	}

	// Get the appropriate RomData class for this ROM.
	const RomDataPtr romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_DPOVERLAY);
	if (!romData) {
		// No RomData.
		return sl;
	}

	// If the ROM image has "dangerous" permissions,
	// return the "security-medium" overlay icon.
	if (romData->hasDangerousPermissions()) {
		sl += QLatin1String("security-medium");
	}

	return sl;
}

} //namespace RomPropertiesKDE
