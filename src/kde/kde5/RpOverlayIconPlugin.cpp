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

// RpFileKio
#include "../RpFile_kio.hpp"

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
		if (getuid() == 0 || geteuid() == 0) {
			qCritical("*** overlayiconplugin_rom-properties-kde%u does not support running as root.", QT_VERSION >> 16);
			return nullptr;
		}

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

	// Check if the source URL is a local file.
	QString qs_source_filename;
	if (item.isLocalFile()) {
		// "file://" scheme. This is a local file.
		qs_source_filename = item.toLocalFile();
	} else if (item.scheme() == QLatin1String("desktop")) {
		// Desktop folder.
		// KFileItem::localPath() isn't working for "desktop:/" here,
		// so handle it manually.
		// TODO: Also handle "trash:/"?
		qs_source_filename = QStandardPaths::locate(QStandardPaths::DesktopLocation, item.path());
	} else {
		// Has a scheme that isn't "file://".
		// This is probably a remote file.
	}

	if (!qs_source_filename.isEmpty()) {
		// Check for "bad" file systems.
		if (FileSystem::isOnBadFS(qs_source_filename.toUtf8().constData(), config->enableThumbnailOnNetworkFS())) {
			// This file is on a "bad" file system.
			return sl;
		}
	}

	// Attempt to open the ROM file.
	IRpFile *file = nullptr;
	if (!qs_source_filename.isEmpty()) {
		// Local file. Use RpFile.
		file = new RpFile(qs_source_filename.toUtf8().constData(), RpFile::FM_OPEN_READ_GZ);
	} else {
#ifdef HAVE_RPFILE_KIO
		// Not a local file. Use RpFileKio.
		file = new RpFileKio(item);
#else /* !HAVE_RPFILE_KIO */
		// RpFileKio is not available.
		return sl;
#endif /* HAVE_RPFILE_KIO */
	}

	if (!file->isOpen()) {
		// Could not open the file.
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
