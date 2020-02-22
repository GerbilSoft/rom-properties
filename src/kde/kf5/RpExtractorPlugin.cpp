/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * RpExtractorPlugin.cpp: KFileMetaData forwarder.                         *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KFileMetaData ExtractorPlugin, *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpExtractorPlugin.hpp"

// librpbase
using namespace LibRpBase;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// C++ STL classes.
using std::string;
using std::vector;

// Qt includes.
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
			qCritical("*** kfilemetadata_rom_properties_" RP_KDE_LOWER "%u does not support running as root.", QT_VERSION >> 16);
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
	// Attempt to open the ROM file.
	IRpFile *const file = openQUrl(QUrl(result->inputUrl()), false);
	if (!file) {
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
