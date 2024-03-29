/***************************************************************************
 * ROM Properties Page shell extension. (GNOME Tracker)                    *
 * rp-tracker.cpp: Tracker extractor module                                *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "tracker-mini.h"

// glib: module handling
#include <gmodule.h>

// GLib on non-Windows platforms (prior to 2.53.1) defines G_MODULE_EXPORT to a no-op.
// This doesn't work when we use symbol visibility settings.
#if !GLIB_CHECK_VERSION(2,53,1) && !defined(_WIN32) && (defined(__GNUC__) && __GNUC__ >= 4)
#  ifdef G_MODULE_EXPORT
#    undef G_MODULE_EXPORT
#  endif
#  define G_MODULE_EXPORT __attribute__((visibility("default")))
#endif /* !GLIB_CHECK_VERSION(2,53,1) && !_WIN32 && __GNUC__ >= 4 */

// libromdata
#include "librpbase/RomData.hpp"
#include "librpbase/RomMetaData.hpp"
#include "libromdata/RomDataFactory.hpp"
using namespace LibRpBase;
using namespace LibRomData;

// C includes (C++ namespace)
#include <cstdio>
#include <stdlib.h>

extern "C"
G_MODULE_EXPORT gboolean
tracker_extract_get_metadata(TrackerExtractInfo *info)
{
	// Make sure the Tracker function pointers are initialized.
	// TODO: ELF ctor/dtor?
	if (rp_tracker_init_pfn() != 0) {
		// Failed to initialize function pointers.
		return false;
	}

	GFile *const file = pfn_tracker_extract_info_get_file(info);
	if (!file) {
		return false;
	}

	// Attempt to open the file using RomDataFactory.
	// TODO: "Slow" FS checking?
	gchar *const filename = g_file_get_path(file);
	RomDataPtr romData = RomDataFactory::create(filename);
	g_free(filename);
	if (!romData) {
		// No RomData was created.
		return false;
	}

	TrackerSparqlBuilder *const metadata = pfn_tracker_extract_info_get_metadata_builder(info);

	// Determine the file type.
	// TODO: Better NFOs for some of these.
	switch (romData->fileType()) {
		default:
			assert(!"Unhandled file type!");
			break;
		case RomData::FileType::ROM_Image:
			pfn_tracker_sparql_builder_predicate(metadata, "a");
			pfn_tracker_sparql_builder_object(metadata, "nfo:Software");
			break;

		case RomData::FileType::DiscImage:
		case RomData::FileType::SaveFile:
		case RomData::FileType::EmbeddedDiscImage:
		case RomData::FileType::ApplicationPackage:
		case RomData::FileType::NFC_Dump:
		case RomData::FileType::DiskImage:
		case RomData::FileType::Executable:
		case RomData::FileType::DLL:
		case RomData::FileType::DeviceDriver:
		case RomData::FileType::ResourceLibrary:
		case RomData::FileType::IconFile:
		case RomData::FileType::BannerFile:
		case RomData::FileType::Homebrew:
		case RomData::FileType::eMMC_Dump:
		case RomData::FileType::ContainerFile:
		case RomData::FileType::FirmwareBinary:

		case RomData::FileType::TextureFile:
			pfn_tracker_sparql_builder_predicate(metadata, "a");
			pfn_tracker_sparql_builder_object(metadata, "nfo:Image");
			pfn_tracker_sparql_builder_object(metadata, "nfo:RasterImage");
			break;

		case RomData::FileType::RelocatableObject:
		case RomData::FileType::SharedLibrary:
		case RomData::FileType::CoreDump:

		case RomData::FileType::AudioFile:
			pfn_tracker_sparql_builder_predicate(metadata, "a");
			pfn_tracker_sparql_builder_object(metadata, "nfo:Audio");
			break;

		case RomData::FileType::BootSector:
		case RomData::FileType::Bundle:
		case RomData::FileType::ResourceFile:
		case RomData::FileType::Partition:
		case RomData::FileType::MetadataFile:
		case RomData::FileType::PatchFile:
		case RomData::FileType::Ticket:
			// TODO
			break;
	}

	// Process metadata properties.
	const RomMetaData *const metaData = romData->metaData();
	if (!metaData) {
		// No metadata properties. Can't do much else here.
		return true;
	}

	const auto iter_end = metaData->cend();
	for (auto iter = metaData->cbegin(); iter != iter_end; ++iter) {
		const RomMetaData::MetaData &prop = *iter;

		switch (prop.name) {
			default:
				// TODO
				break;

			// Audio
			case Property::Channels:
				pfn_tracker_sparql_builder_predicate(metadata, "nfo:channels");
				pfn_tracker_sparql_builder_object_int64(metadata, prop.data.ivalue);
				break;
			case Property::Duration:
				// NOTE: RomMetaData uses milliseconds. Tracker uses seconds.
				pfn_tracker_sparql_builder_predicate(metadata, "nfo:duration");
				pfn_tracker_sparql_builder_object_int64(metadata, prop.data.ivalue / 1000);
				break;
			case Property::Genre:
				pfn_tracker_sparql_builder_predicate(metadata, "nmm:genre");
				pfn_tracker_sparql_builder_object_string(metadata, prop.data.str->c_str());
				break;
			case Property::SampleRate:
				pfn_tracker_sparql_builder_predicate(metadata, "nfo:sampleRate");
				pfn_tracker_sparql_builder_object_int64(metadata, prop.data.ivalue);
				break;
			case Property::TrackNumber:
				pfn_tracker_sparql_builder_predicate(metadata, "nmm:trackNumber");
				pfn_tracker_sparql_builder_object_int64(metadata, prop.data.ivalue);
				break;
			case Property::ReleaseYear:
				// FIXME: Needs to be in nie:informationElementDate foramt.
				/*
				pfn_tracker_sparql_builder_predicate(metadata, "nmm:releaseDate");
				pfn_tracker_sparql_builder_object_int64(metadata, prop.data.ivalue);
				*/
				break;
			case Property::Artist:
				// TODO
				break;
			case Property::Album:
				pfn_tracker_sparql_builder_predicate(metadata, "nmm:musicAlbum");
				pfn_tracker_sparql_builder_object_string(metadata, prop.data.str->c_str());
				break;
			case Property::AlbumArtist:
				// TODO
				break;
			case Property::Composer:
				pfn_tracker_sparql_builder_predicate(metadata, "nmm:composer");
				pfn_tracker_sparql_builder_object_string(metadata, prop.data.str->c_str());
				break;
			case Property::Lyricist:
				pfn_tracker_sparql_builder_predicate(metadata, "nmm:lyricist");
				pfn_tracker_sparql_builder_object_string(metadata, prop.data.str->c_str());
				break;

			// Document
			case Property::Author:
				// NOTE: Closest equivalent is "Creator".
				pfn_tracker_sparql_builder_predicate(metadata, "nco:creator");
				pfn_tracker_sparql_builder_object_string(metadata, prop.data.str->c_str());
				break;
			case Property::Title:
				pfn_tracker_sparql_builder_predicate(metadata, "nie:title");
				pfn_tracker_sparql_builder_object_string(metadata, prop.data.str->c_str());
				break;
			case Property::Copyright:
				pfn_tracker_sparql_builder_predicate(metadata, "nie:copyright");
				pfn_tracker_sparql_builder_object_string(metadata, prop.data.str->c_str());
				break;
			case Property::Publisher:
				pfn_tracker_sparql_builder_predicate(metadata, "nco:publisher");
				pfn_tracker_sparql_builder_object_string(metadata, prop.data.str->c_str());
				break;
			case Property::Description:
				pfn_tracker_sparql_builder_predicate(metadata, "nie:description");
				pfn_tracker_sparql_builder_object_string(metadata, prop.data.str->c_str());
				break;
			case Property::CreationDate:
				// TODO: Convert from Unix timestamp to "xsd:dateTime" for "nie:contentCreated".
				break;

			// Media
			case Property::Width:
				pfn_tracker_sparql_builder_predicate(metadata, "nfo:width");
				pfn_tracker_sparql_builder_object_int64(metadata, prop.data.ivalue);
				break;
			case Property::Height:
				pfn_tracker_sparql_builder_predicate(metadata, "nfo:height");
				pfn_tracker_sparql_builder_object_int64(metadata, prop.data.ivalue);
				break;

			// Audio
			case Property::DiscNumber:
				pfn_tracker_sparql_builder_predicate(metadata, "nmm:setNumber");
				pfn_tracker_sparql_builder_object_int64(metadata, prop.data.ivalue);
				break;
		}
	}

	return true;
}
