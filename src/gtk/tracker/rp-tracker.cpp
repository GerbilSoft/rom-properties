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

	GFile *const file = tracker_extract_pfns.v1.info.get_file(info);
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

	TrackerSparqlBuilder *const metadata = tracker_extract_pfns.v1.info.get_metadata_builder(info);

	// Determine the file type.
	// TODO: Better NFOs for some of these.
	switch (romData->fileType()) {
		default:
			assert(!"Unhandled file type!");
			break;
		case RomData::FileType::ROM_Image:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Software");
			break;
		case RomData::FileType::DiscImage:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Filesystem");
			// TODO: Specific type of file system? ("nfo:filesystemType")
			break;
		case RomData::FileType::SaveFile:
			// TODO
			break;
		case RomData::FileType::EmbeddedDiscImage:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Filesystem");
			// TODO: Specific type of file system? ("nfo:filesystemType")
			break;
		case RomData::FileType::ApplicationPackage:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Application");
		case RomData::FileType::NFC_Dump:
			// TODO
			break;
		case RomData::FileType::DiskImage:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Filesystem");
			// TODO: Specific type of file system? ("nfo:filesystemType")
			break;
		case RomData::FileType::Executable:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Application");
			break;
		case RomData::FileType::DLL:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Software");
			break;
		case RomData::FileType::DeviceDriver:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Software");
			break;
		case RomData::FileType::ResourceLibrary:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:DataContainer");
			break;
		case RomData::FileType::IconFile:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Image");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:RasterImage");
			break;
		case RomData::FileType::BannerFile:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Image");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:RasterImage");
			break;
		case RomData::FileType::Homebrew:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Application");
			break;
		case RomData::FileType::eMMC_Dump:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Filesystem");
			break;
		case RomData::FileType::ContainerFile:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:DataContainer");
			break;
		case RomData::FileType::FirmwareBinary:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:OperatingSystem");
			break;
		case RomData::FileType::TextureFile:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Image");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:RasterImage");
			break;
		case RomData::FileType::RelocatableObject:
			// TODO
			break;
		case RomData::FileType::SharedLibrary:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Application");
			break;
		case RomData::FileType::CoreDump:
			// TODO
			break;
		case RomData::FileType::AudioFile:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:Audio");
			break;
		case RomData::FileType::BootSector:
			// TODO
			break;
		case RomData::FileType::Bundle:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:DataContainer");
			break;
		case RomData::FileType::ResourceFile:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:DataContainer");
			break;
		case RomData::FileType::Partition:
			tracker_sparql_pfns.v1.builder.predicate(metadata, "a");
			tracker_sparql_pfns.v1.builder.object(metadata, "nfo:FilesystemImage");
			break;

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
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nfo:channels");
				tracker_sparql_pfns.v1.builder.object_int64(metadata, prop.data.ivalue);
				break;
			case Property::Duration:
				// NOTE: RomMetaData uses milliseconds. Tracker uses seconds.
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nfo:duration");
				tracker_sparql_pfns.v1.builder.object_int64(metadata, prop.data.ivalue / 1000);
				break;
			case Property::Genre:
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nmm:genre");
				tracker_sparql_pfns.v1.builder.object_string(metadata, prop.data.str->c_str());
				break;
			case Property::SampleRate:
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nfo:sampleRate");
				tracker_sparql_pfns.v1.builder.object_int64(metadata, prop.data.ivalue);
				break;
			case Property::TrackNumber:
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nmm:trackNumber");
				tracker_sparql_pfns.v1.builder.object_int64(metadata, prop.data.ivalue);
				break;
			case Property::ReleaseYear:
				// FIXME: Needs to be in nie:informationElementDate foramt.
				/*
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nmm:releaseDate");
				tracker_sparql_pfns.v1.builder.object_int64(metadata, prop.data.ivalue);
				*/
				break;
			case Property::Artist:
				// TODO
				break;
			case Property::Album:
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nmm:musicAlbum");
				tracker_sparql_pfns.v1.builder.object_string(metadata, prop.data.str->c_str());
				break;
			case Property::AlbumArtist:
				// TODO
				break;
			case Property::Composer:
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nmm:composer");
				tracker_sparql_pfns.v1.builder.object_string(metadata, prop.data.str->c_str());
				break;
			case Property::Lyricist:
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nmm:lyricist");
				tracker_sparql_pfns.v1.builder.object_string(metadata, prop.data.str->c_str());
				break;

			// Document
			case Property::Author:
				// NOTE: Closest equivalent is "Creator".
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nco:creator");
				tracker_sparql_pfns.v1.builder.object_string(metadata, prop.data.str->c_str());
				break;
			case Property::Title:
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nie:title");
				tracker_sparql_pfns.v1.builder.object_string(metadata, prop.data.str->c_str());
				break;
			case Property::Copyright:
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nie:copyright");
				tracker_sparql_pfns.v1.builder.object_string(metadata, prop.data.str->c_str());
				break;
			case Property::Publisher:
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nco:publisher");
				tracker_sparql_pfns.v1.builder.object_string(metadata, prop.data.str->c_str());
				break;
			case Property::Description:
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nie:description");
				tracker_sparql_pfns.v1.builder.object_string(metadata, prop.data.str->c_str());
				break;
			case Property::CreationDate:
				// TODO: Convert from Unix timestamp to "xsd:dateTime" for "nie:contentCreated".
				break;

			// Media
			case Property::Width:
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nfo:width");
				tracker_sparql_pfns.v1.builder.object_int64(metadata, prop.data.ivalue);
				break;
			case Property::Height:
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nfo:height");
				tracker_sparql_pfns.v1.builder.object_int64(metadata, prop.data.ivalue);
				break;

			// Audio
			case Property::DiscNumber:
				tracker_sparql_pfns.v1.builder.predicate(metadata, "nmm:setNumber");
				tracker_sparql_pfns.v1.builder.object_int64(metadata, prop.data.ivalue);
				break;
		}
	}

	return true;
}
