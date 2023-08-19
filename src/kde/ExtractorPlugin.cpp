/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ExtractorPlugin.cpp: KFileMetaData extractor plugin.                    *
 *                                                                         *
 * Qt's plugin system prevents a single shared library from exporting      *
 * multiple plugins, so this file acts as a KFileMetaData ExtractorPlugin, *
 * and then forwards the request to the main library.                      *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ExtractorPlugin.hpp"
#include "check-uid.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// C++ STL classes
using std::string;
using std::vector;

// KDE includes
// NOTE: kfilemetadata_version.h was added in 5.94.0, so we can't use it.
// Using kcoreaddons_version.h instead.
#include <kfileitem.h>
//#include <kfilemetadata_version.h>
#include <kcoreaddons_version.h>
#include <kfilemetadata/extractorplugin.h>
#include <kfilemetadata/properties.h>
using KFileMetaData::ExtractionResult;
using namespace KFileMetaData::Property;

/**
 * Factory method.
 * NOTE: Unlike the ThumbCreator version, this one is specific to
 * rom-properties, and is called by a forwarder library.
 */
extern "C" {
	Q_DECL_EXPORT RomPropertiesKDE::ExtractorPlugin *PFN_CREATEEXTRACTORPLUGINKDE_FN(QObject *parent)
	{
		CHECK_UID_RET(nullptr);
		return new RomPropertiesKDE::ExtractorPlugin(parent);
	}
}

namespace RomPropertiesKDE {

ExtractorPlugin::ExtractorPlugin(QObject *parent)
	: super(parent)
{ }

QStringList ExtractorPlugin::mimetypes(void) const
{
	// Get the MIME types from RomDataFactory.
	const vector<const char*> &vec_mimeTypes = RomDataFactory::supportedMimeTypes();

	// Convert to QStringList.
	QStringList mimeTypes;
	mimeTypes.reserve(static_cast<int>(vec_mimeTypes.size()));
	for (const char *mimeType : vec_mimeTypes) {
		mimeTypes += QLatin1String(mimeType);
	}
	return mimeTypes;
}

void ExtractorPlugin::extract_properties(KFileMetaData::ExtractionResult *result, RomData *romData)
{
	const RomMetaData *const metaData = romData->metaData();
	if (!metaData || metaData->empty()) {
		// No metadata properties.
		return;
	}

	// Process the metadata.
	const auto iter_end = metaData->cend();
	for (auto iter = metaData->cbegin(); iter != iter_end; ++iter) {
		const RomMetaData::MetaData &prop = *iter;

		// RomMetaData's property indexes match KFileMetaData.
		// No conversion is necessary.
		switch (prop.type) {
			case PropertyType::Integer: {
				int ivalue = prop.data.ivalue;
				switch (prop.name) {
					case LibRpBase::Property::Duration:
						// rom-properties: milliseconds
						// KFileMetaData: seconds
						ivalue /= 1000;
						break;
					case LibRpBase::Property::Rating:
						// rom-properties: [0,100]
						// KFileMetaData: [0,10]
						ivalue /= 10;
						break;
					default:
						break;
				}
				result->add(static_cast<KFileMetaData::Property::Property>(prop.name), ivalue);
				break;
			}

			case PropertyType::UnsignedInteger: {
				result->add(static_cast<KFileMetaData::Property::Property>(prop.name),
					prop.data.uvalue);
				break;
			}

			case PropertyType::String: {
				LibRpBase::Property prop_name = prop.name;
				// NOTE: kfilemetadata_version.h was added in KF5 5.94.0.
				// Using kcoreaddons_version.h instead.
#if KCOREADDONS_VERSION < QT_VERSION_CHECK(5,53,0)
				if (prop_name == LibRpBase::Property::Description) {
					// KF5 5.53 added Description.
					// Fall back to Subject since Description isn't available.
					prop_name = LibRpBase::Property::Subject;
				}
#endif /* KCOREADDONS_VERSION < QT_VERSION_CHECK(5,53,0) */

				const string *str = prop.data.str;
				if (str) {
					result->add(static_cast<KFileMetaData::Property::Property>(prop_name),
						QString::fromUtf8(str->data(), static_cast<int>(str->size())));
				}
				break;
			}

			case PropertyType::Timestamp: {
				// TODO: Verify timezone handling.
				// NOTE: fromMSecsSinceEpoch() with TZ spec was added in Qt 5.2.
				// Maybe write a wrapper function? (RomDataView uses this, too.)
				// NOTE: Some properties might need the full QDateTime.
				// CreationDate seems to work fine with just QDate.
				QDateTime dateTime;
				dateTime.setTimeSpec(Qt::UTC);
				dateTime.setMSecsSinceEpoch((qint64)prop.data.timestamp * 1000);
				result->add(static_cast<KFileMetaData::Property::Property>(prop.name),
					dateTime.date());
				break;
			}

			case PropertyType::Double: {
				result->add(static_cast<KFileMetaData::Property::Property>(prop.name),
					prop.data.dvalue);
				break;
			}

			default:
				// ERROR!
				assert(!"Unsupported RomMetaData PropertyType.");
				break;
		}
	}
}

#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5,76,0)
void ExtractorPlugin::extract_image(KFileMetaData::ExtractionResult *result, RomData *romData)
{
	// TODO: Get external images. For now, only using internal images.
	// Image data should be a PNG or JPEG file.

	// File Icon (IMG_INT_ICON)
	// - If not available: IMG_INT_BANNER

	// Front Cover (IMG_EXT_COVER)
	// Media (IMG_EXT_MEDIA)

	// For "Other", use the following, if available:
	// - IMG_INT_IMAGE
	// - IMG_EXT_TITLE_SCREEN

	/** TODO: Implement this. Placeholder for now... **/
	Q_UNUSED(result)
	Q_UNUSED(romData)
}
#endif /* KCOREADDONS_VERSION >= QT_VERSION_CHECK(5,76,0) */

void ExtractorPlugin::extract(ExtractionResult *result)
{
	const ExtractionResult::Flags flags = result->inputFlags();
	if (unlikely(flags == ExtractionResult::ExtractNothing)) {
		// Nothing to extract...
		return;
	}

	// Attempt to open the ROM file.
	const IRpFilePtr file(openQUrl(QUrl(result->inputUrl()), false));
	if (!file) {
		// Could not open the file.
		return;
	}

	// Which attributes are required?
#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5,76,0)
	static const unsigned int mask = ExtractionResult::ExtractMetaData | ExtractionResult::ExtractImageData;
#else /* KCOREADDONS_VERSION < QT_VERSION_CHECK(5,76,0) */
	static const unsigned int mask = ExtractionResult::ExtractMetaData;
#endif /* KCOREADDONS_VERSION >= QT_VERSION_CHECK(5,76,0) */
	unsigned int attrs;
	switch (flags & mask) {
		case ExtractionResult::ExtractMetaData:
			// Only extract metadata.
			attrs = RomDataFactory::RDA_HAS_METADATA;
			break;
#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5,76,0)
		case ExtractionResult::ExtractImageData:
			// Only extract images.
			attrs = RomDataFactory::RDA_HAS_THUMBNAIL;
			break;
#endif /* KCOREADDONS_VERSION >= QT_VERSION_CHECK(5,76,0) */
		default:
			// Multiple things to extract.
			attrs = 0;
			break;
	}

	// Get the appropriate RomData class for this ROM.
	// file is dup()'d by RomData.
	RomData *const romData = RomDataFactory::create(file, attrs);
	if (!romData) {
		// ROM is not supported.
		return;
	}

	// File type
	// NOTE: KFileMetaData has a limited set of file types as of v5.107.
	static_assert((int)RomData::FileType::Max == (int)RomData::FileType::PatchFile + 1, "Update KFileMetaData file types!");
	switch (romData->fileType()) {
		default:
			// No KFileMetaData::Type is applicable here.
			break;

		case RomData::FileType::IconFile:
		case RomData::FileType::BannerFile:
		case RomData::FileType::TextureFile:
			result->addType(KFileMetaData::Type::Image);
			break;

		case RomData::FileType::ContainerFile:
		case RomData::FileType::Bundle:
			result->addType(KFileMetaData::Type::Archive);
			break;

		case RomData::FileType::AudioFile:
			result->addType(KFileMetaData::Type::Audio);
			break;
	}

	// Metadata properties
	if (flags & ExtractionResult::ExtractMetaData) {
		extract_properties(result, romData);
	}

#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5,76,0)
	// KFileMetaData 5.76.0 added images.
	if (flags & ExtractionResult::ExtractImageData) {
		extract_image(result, romData);
	}
#endif /* KCOREADDONS_VERSION >= QT_VERSION_CHECK(5,76,0) */

	// Finished extracting metadata.
	romData->unref();
}

}
