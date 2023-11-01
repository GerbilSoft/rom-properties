/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * OverlayIconPlugin.cpp: KOverlayIconPlugin.                              *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KOverlayIconPlugin,            *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "OverlayIconPlugin.hpp"
#include "check-uid.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// C++ STL classes
using std::string;

// Qt includes.
#include <QtCore/QStandardPaths>

/**
 * Factory method.
 * NOTE: Unlike the ThumbCreator version, this one is specific to
 * rom-properties, and is called by a forwarder library.
 */
extern "C" {
	Q_DECL_EXPORT RomPropertiesKDE::OverlayIconPlugin *PFN_CREATEOVERLAYICONPLUGINKDE_FN(QObject *parent)
	{
		CHECK_UID_RET(nullptr);
		return new RomPropertiesKDE::OverlayIconPlugin(parent);
	}
}

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
