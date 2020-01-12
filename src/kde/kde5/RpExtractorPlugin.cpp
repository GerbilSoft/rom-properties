/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * RpExtractorPlugin.cpp: KFileMetaData forwarder.                         *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KFileMetaData ExtractorPlugin, *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpExtractorPlugin.hpp"
#include "RpQt.hpp"

// librpbase
#include "librpbase/RomData.hpp"
#include "librpbase/RomMetaData.hpp"
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
#include <algorithm>
#include <string>
#include <vector>
using std::string;
using std::vector;

// Qt includes.
#include <QtCore/QDateTime>
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
	Q_DECL_EXPORT RpExtractorPlugin *PFN_CREATEEXTRACTORPLUGINKDE_FN(QObject *parent)
	{
		if (getuid() == 0 || geteuid() == 0) {
			qCritical("*** kfilemetadata_rom-properties-kde%u does not support running as root.", QT_VERSION >> 16);
			return nullptr;
		}

		return new RpExtractorPlugin(parent);
	}
}

RpExtractorPlugin::RpExtractorPlugin(QObject *parent)
	: super(parent)
{ }

QStringList RpExtractorPlugin::mimetypes(void) const
{
	// Get the MIME types from RomDataFactory.
	const vector<const char*> &vec_mimeTypes = RomDataFactory::supportedMimeTypes();

	// Convert to QStringList.
	QStringList mimeTypes;
	mimeTypes.reserve(static_cast<int>(vec_mimeTypes.size()));
	std::for_each(vec_mimeTypes.cbegin(), vec_mimeTypes.cend(),
		[&mimeTypes](const char *mimeType) {
			mimeTypes += QString::fromUtf8(mimeType);
		}
	);
	return mimeTypes;
}

void RpExtractorPlugin::extract(ExtractionResult *result)
{
	// Check if the source filename is a URI.
	// FIXME: Use KFileItem to handle "desktop:/" as a local file.
	const QString source_file = result->inputUrl();
	QUrl url(source_file);
	QFileInfo fi_src;
	QString qs_source_filename;
	if (url.scheme().isEmpty()) {
		// No scheme. This is a plain old filename.
		fi_src = QFileInfo(source_file);
		qs_source_filename = fi_src.absoluteFilePath();
		url = QUrl::fromLocalFile(qs_source_filename);
	} else if (url.isLocalFile()) {
		// "file://" scheme. This is a local file.
		qs_source_filename = url.toLocalFile();
		fi_src = QFileInfo(qs_source_filename);
		url = QUrl::fromLocalFile(fi_src.absoluteFilePath());
	} else if (url.scheme() == QLatin1String("desktop")) {
		// Desktop folder.
		// KFileItem::localPath() isn't working for "desktop:/" here,
		// so handle it manually.
		// TODO: Also handle "trash:/"?
		qs_source_filename = QStandardPaths::locate(QStandardPaths::DesktopLocation, url.path());
	} else {
		// Has a scheme that isn't "file://".
		// This is probably a remote file.
	}

	if (!qs_source_filename.isEmpty()) {
		// Check for "bad" file systems.
		const Config *const config = Config::instance();
		if (FileSystem::isOnBadFS(qs_source_filename.toUtf8().constData(), config->enableThumbnailOnNetworkFS())) {
			// This file is on a "bad" file system.
			return;
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
		file = new RpFileKio(url);
#else /* !HAVE_RPFILE_KIO */
		// RpFileKio is not available.
		return;
#endif /* HAVE_RPFILE_KIO */
	}

	if (!file->isOpen()) {
		// Could not open the file.
		return;
	}

	// Get the appropriate RomData class for this ROM.
	// file is dup()'d by RomData.
	RomData *const romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_METADATA);
	file->unref();	// file is ref()'d by RomData.
	if (!romData) {
		// ROM is not supported.
		return;
	}

	// Get the metadata properties.
	const RomMetaData *const metaData = romData->metaData();
	if (!metaData || metaData->empty()) {
		// No metadata properties.
		romData->unref();
		return;
	}

	// Process the metadata.
	const int count = metaData->count();
	for (int i = 0; i < count; i++) {
		const RomMetaData::MetaData *const prop = metaData->prop(i);
		assert(prop != nullptr);
		if (!prop)
			continue;

		// RomMetaData's property indexes match KFileMetaData.
		// No conversion is necessary.
		switch (prop->type) {
			case LibRpBase::PropertyType::Integer: {
				int ivalue = prop->data.ivalue;
				if (prop->name == LibRpBase::Property::Duration) {
					// Duration needs to be converted from ms to seconds.
					ivalue /= 1000;
				}
				result->add(static_cast<KFileMetaData::Property::Property>(prop->name), ivalue);
				break;
			}

			case LibRpBase::PropertyType::UnsignedInteger: {
				result->add(static_cast<KFileMetaData::Property::Property>(prop->name),
					    prop->data.uvalue);
				break;
			}

			case LibRpBase::PropertyType::String: {
				const string *str = prop->data.str;
				if (str) {
					result->add(static_cast<KFileMetaData::Property::Property>(prop->name),
						QString::fromUtf8(str->data(), static_cast<int>(str->size())));
				}
				break;
			}

			case LibRpBase::PropertyType::Timestamp: {
				// TODO: Verify timezone handling.
				// NOTE: fromMSecsSinceEpoch() with TZ spec was added in Qt 5.2.
				// Maybe write a wrapper function? (RomDataView uses this, too.)
				// NOTE: Some properties might need the full QDateTime.
				// CreationDate seems to work fine with just QDate.
				QDateTime dateTime;
				dateTime.setTimeSpec(Qt::UTC);
				dateTime.setMSecsSinceEpoch((qint64)prop->data.timestamp * 1000);
				result->add(static_cast<KFileMetaData::Property::Property>(prop->name),
					dateTime.date());
				break;
			}

			default:
				// ERROR!
				assert(!"Unsupported RomMetaData PropertyType.");
				break;
		}
	}

	// Finished extracting metadata.
	romData->unref();
}

}

#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
