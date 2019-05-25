/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * RpOverlayIconPlugin.cpp: KOverlayIconPlugin.                            *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KOverlayIconPlugin,            *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpOverlayIconPlugin.hpp"
#include "RpQt.hpp"

// librpbase
#include "librpbase/RomData.hpp"
#include "librpbase/file/FileSystem.hpp"
#include "librpbase/file/RpFile.hpp"
#include "librpbase/config/Config.hpp"
using namespace LibRpBase;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

// Qt includes.
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>

// KDE includes.
#include <kfilemetadata/extractorplugin.h>
#include <kfilemetadata/properties.h>
using KFileMetaData::ExtractorPlugin;
using KFileMetaData::ExtractionResult;
using namespace KFileMetaData::Property;

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <kfileitem.h>

namespace RomPropertiesKDE {

/**
 * Factory method.
 * NOTE: Unlike the ThumbCreator version, this one is specific to
 * rom-properties, and is called by a forwarder library.
 */
extern "C" {
	Q_DECL_EXPORT RpOverlayIconPlugin *PFN_CREATEOVERLAYICONPLUGINKDE_FN(QObject *parent)
	{
		return new RpOverlayIconPlugin(parent);
	}
}

RpOverlayIconPlugin::RpOverlayIconPlugin(QObject *parent)
	: super(parent)
{ }

QStringList RpOverlayIconPlugin::getOverlays(const QUrl &item)
{
	// TODO: Check for slow devices and/or cache this?
	QStringList sl;

	const Config *const config = Config::instance();
	if (!config->showDangerousPermissionsOverlayIcon()) {
		// Overlay icon is disabled.
		return sl;
	}

	// FIXME: KFileItem's localPath() isn't working here for some reason.
	// We'll handle desktop:/ manually.
	QString filename = item.toLocalFile();
	if (filename.isEmpty()) {
		// Unable to convert it directly.
		// Check for "desktop:/".
		const QString scheme = item.scheme();
		if (scheme == QLatin1String("desktop")) {
			// Desktop folder.
			// TODO: Remove leading '/' from item.path()?
			filename = QStandardPaths::locate(QStandardPaths::DesktopLocation, item.path());
		} else {
			// Unsupported scheme.
			return sl;
		}
	}

	if (filename.isEmpty()) {
		// No filename.
		return sl;
	}

	// Single file, and it's local.
	const QByteArray u8filename = filename.toUtf8();

	// Check for "bad" file systems.
	if (FileSystem::isOnBadFS(u8filename.constData(), config->enableThumbnailOnNetworkFS())) {
		// This file is on a "bad" file system.
		return sl;
	}

	// TODO: RpQFile wrapper.
	// For now, using RpFile, which is an stdio wrapper.
	RpFile *const file = new RpFile(u8filename.constData(), RpFile::FM_OPEN_READ_GZ);
	if (!file->isOpen()) {
		// Could not open the file.
		file->unref();
		return sl;
	}

	// Get the appropriate RomData class for this ROM.
	// file is dup()'d by RomData.
	RomData *const romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_DPOVERLAY);
	file->unref();	// file is ref()'d by RomData.
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

}

#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
