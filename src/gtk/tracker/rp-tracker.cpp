/***************************************************************************
 * ROM Properties Page shell extension. (GNOME Tracker)                    *
 * rp-tracker.cpp: Tracker extractor module                                *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "tracker-mini.h"
#include "tracker-file-utils.h"

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

static void
add_metadata_properties_v1(const RomMetaData *metaData, TrackerSparqlBuilder *builder)
{
	const auto iter_end = metaData->cend();
	for (auto iter = metaData->cbegin(); iter != iter_end; ++iter) {
		const RomMetaData::MetaData &prop = *iter;

		switch (prop.name) {
			default:
				// TODO
				break;

			// Audio
			case Property::Channels:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nfo:channels");
				tracker_sparql_pfns.v1.builder.object_int64(builder, prop.data.ivalue);
				break;
			case Property::Duration:
				// NOTE: RomMetaData uses milliseconds. Tracker uses seconds.
				tracker_sparql_pfns.v1.builder.predicate(builder, "nfo:duration");
				tracker_sparql_pfns.v1.builder.object_int64(builder, prop.data.ivalue / 1000);
				break;
			case Property::Genre:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nmm:genre");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str->c_str());
				break;
			case Property::SampleRate:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nfo:sampleRate");
				tracker_sparql_pfns.v1.builder.object_int64(builder, prop.data.ivalue);
				break;
			case Property::TrackNumber:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nmm:trackNumber");
				tracker_sparql_pfns.v1.builder.object_int64(builder, prop.data.ivalue);
				break;
			case Property::ReleaseYear:
				// FIXME: Needs to be in nie:informationElementDate foramt.
				/*
				tracker_sparql_pfns.v1.builder.predicate(builder, "nmm:releaseDate");
				tracker_sparql_pfns.v1.builder.object_int64(builder, prop.data.ivalue);
				*/
				break;
			case Property::Artist:
				// TODO
				break;
			case Property::Album:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nmm:musicAlbum");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str->c_str());
				break;
			case Property::AlbumArtist:
				// TODO
				break;
			case Property::Composer:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nmm:composer");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str->c_str());
				break;
			case Property::Lyricist:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nmm:lyricist");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str->c_str());
				break;

			// Document
			case Property::Author:
				// NOTE: Closest equivalent is "Creator".
				tracker_sparql_pfns.v1.builder.predicate(builder, "nco:creator");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str->c_str());
				break;
			case Property::Title:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nie:title");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str->c_str());
				break;
			case Property::Copyright:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nie:copyright");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str->c_str());
				break;
			case Property::Publisher:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nco:publisher");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str->c_str());
				break;
			case Property::Description:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nie:description");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str->c_str());
				break;
			case Property::CreationDate:
				// TODO: Convert from Unix timestamp to "xsd:dateTime" for "nie:contentCreated".
				break;

			// Media
			case Property::Width:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nfo:width");
				tracker_sparql_pfns.v1.builder.object_int64(builder, prop.data.ivalue);
				break;
			case Property::Height:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nfo:height");
				tracker_sparql_pfns.v1.builder.object_int64(builder, prop.data.ivalue);
				break;

			// Audio
			case Property::DiscNumber:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nmm:setNumber");
				tracker_sparql_pfns.v1.builder.object_int64(builder, prop.data.ivalue);
				break;
		}
	}
}

static void
add_metadata_properties_v2(const RomMetaData *metaData, TrackerResource *resource)
{
	// TODO: Make use of tracker_resource_set_relation(), like in tracker-extract-mp3.c?
	const auto iter_end = metaData->cend();
	for (auto iter = metaData->cbegin(); iter != iter_end; ++iter) {
		const RomMetaData::MetaData &prop = *iter;

		switch (prop.name) {
			default:
				// TODO
				break;

			// Audio
			case Property::Channels:
				tracker_sparql_pfns.v2.resource.set_int(resource, "nfo:channels", prop.data.ivalue);
				break;
			case Property::Duration:
				// NOTE: RomMetaData uses milliseconds. Tracker uses seconds.
				tracker_sparql_pfns.v2.resource.set_int(resource, "nfo:duration", prop.data.ivalue / 1000);
				break;
			case Property::Genre:
				tracker_sparql_pfns.v2.resource.set_string(resource, "nmm:genre", prop.data.str->c_str());
				break;
			case Property::SampleRate:
				tracker_sparql_pfns.v2.resource.set_int(resource, "nfo:sampleRate", prop.data.ivalue);
				break;
			case Property::TrackNumber:
				tracker_sparql_pfns.v2.resource.set_int(resource, "nmm:trackNumber", prop.data.ivalue);
				break;
			case Property::ReleaseYear:
				// FIXME: Needs to be in nie:informationElementDate foramt.
				/*
				tracker_sparql_pfns.v2.resource.set_string(resource, "nmm:releaseDate", prop.data.ivalue);
				*/
				break;
			case Property::Artist:
				// TODO
				break;
			case Property::Album:
				tracker_sparql_pfns.v2.resource.set_string(resource, "nmm:musicAlbum", prop.data.str->c_str());
				break;
			case Property::AlbumArtist:
				// TODO
				break;
			case Property::Composer:
				tracker_sparql_pfns.v2.resource.set_string(resource, "nmm:composer", prop.data.str->c_str());
				break;
			case Property::Lyricist:
				tracker_sparql_pfns.v2.resource.set_string(resource, "nmm:lyricist", prop.data.str->c_str());
				break;

			// Document
			case Property::Author:
				// NOTE: Closest equivalent is "Creator".
				tracker_sparql_pfns.v2.resource.set_string(resource, "nco:creator", prop.data.str->c_str());
				break;
			case Property::Title:
				tracker_sparql_pfns.v2.resource.set_string(resource, "nie:title", prop.data.str->c_str());
				break;
			case Property::Copyright:
				tracker_sparql_pfns.v2.resource.set_string(resource, "nie:copyright", prop.data.str->c_str());
				break;
			case Property::Publisher:
				tracker_sparql_pfns.v2.resource.set_string(resource, "nco:publisher", prop.data.str->c_str());
				break;
			case Property::Description:
				tracker_sparql_pfns.v2.resource.set_string(resource, "nie:description", prop.data.str->c_str());
				break;
			case Property::CreationDate:
				// TODO: Convert from Unix timestamp to "xsd:dateTime" for "nie:contentCreated".
				break;

			// Media
			case Property::Width:
				tracker_sparql_pfns.v2.resource.set_int(resource, "nfo:width", prop.data.ivalue);
				break;
			case Property::Height:
				tracker_sparql_pfns.v2.resource.set_int(resource, "nfo:height", prop.data.ivalue);
				break;

			// Audio
			case Property::DiscNumber:
				tracker_sparql_pfns.v2.resource.set_int(resource, "nmm:setNumber", prop.data.ivalue);
				break;
		}
	}
}

// NOTE: The "error" parameter was added in tracker-3.0.
extern "C"
G_MODULE_EXPORT gboolean
tracker_extract_get_metadata(TrackerExtractInfo *info, GError **error)
{
	// Make sure the Tracker function pointers are initialized.
	// TODO: ELF ctor/dtor?
	if (rp_tracker_init_pfn() != 0) {
		// Failed to initialize function pointers.
		return false;
	}

	GFile *const file = tracker_extract_pfns.v1.info.get_file(info);
	if (!file) {
		// TODO: Set error if Tracker 3.0.
		((void)error);
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

	union {
		TrackerSparqlBuilder *builder;
		TrackerResource *resource;
	};
	switch (rp_tracker_api) {
		default:
			assert(!"Tracker API version is not supported.");
			break;

		case 1:
			builder = tracker_extract_pfns.v1.info.get_metadata_builder(info);
			break;

		case 2:
			resource = tracker_sparql_pfns.v2.resource._new(nullptr);
			break;

		case 3: {
			// NOTE: Only using tracker_file_get_content_identifier() if this is API version 3.
			// tracker_file_get_content_identifier() was added to Tracker 3.3.0-alpha.
			// TODO: Only if we're using Tracker 3.3.0 or later?
			gchar *const resource_uri = tracker_file_get_content_identifier(file, NULL, NULL);
			resource = tracker_sparql_pfns.v2.resource._new(resource_uri);
			g_free(resource_uri);
			break;
		}
	}

	// Determine the file type.
	// TODO: Better NFOs for some of these.
	const char *fileTypes[2] = {nullptr, nullptr};
	switch (romData->fileType()) {
		default:
			assert(!"Unhandled file type!");
			break;
		case RomData::FileType::ROM_Image:
			fileTypes[0] = "nfo:Software";
			break;
		case RomData::FileType::DiscImage:
			fileTypes[0] = "nfo:Filesystem";
			// TODO: Specific type of file system? ("nfo:filesystemType")
			break;
		case RomData::FileType::SaveFile:
			// FIXME: Not the best type...
			fileTypes[0] = "nfo:Document";
			break;
		case RomData::FileType::EmbeddedDiscImage:
			fileTypes[0] = "nfo:Filesystem";
			// TODO: Specific type of file system? ("nfo:filesystemType")
			break;
		case RomData::FileType::ApplicationPackage:
			fileTypes[0] = "nfo:Application";
		case RomData::FileType::NFC_Dump:
			// TODO
			break;
		case RomData::FileType::DiskImage:
			fileTypes[0] = "nfo:Filesystem";
			// TODO: Specific type of file system? ("nfo:filesystemType")
			break;
		case RomData::FileType::Executable:
			fileTypes[0] = "nfo:Application";
			break;
		case RomData::FileType::DLL:
			fileTypes[0] = "nfo:Software";
			break;
		case RomData::FileType::DeviceDriver:
			fileTypes[0] = "nfo:Software";
			break;
		case RomData::FileType::ResourceLibrary:
			fileTypes[0] = "nfo:DataContainer";
			break;
		case RomData::FileType::IconFile:
			fileTypes[0] = "nfo:Image";
			fileTypes[1] = "nfo:RasterImage";
			break;
		case RomData::FileType::BannerFile:
			fileTypes[0] = "nfo:Image";
			fileTypes[1] = "nfo:RasterImage";
			break;
		case RomData::FileType::Homebrew:
			fileTypes[0] = "nfo:Application";
			break;
		case RomData::FileType::eMMC_Dump:
			fileTypes[0] = "nfo:Filesystem";
			break;
		case RomData::FileType::ContainerFile:
			fileTypes[0] = "nfo:DataContainer";
			break;
		case RomData::FileType::FirmwareBinary:
			fileTypes[0] = "nfo:OperatingSystem";
			break;
		case RomData::FileType::TextureFile:
			fileTypes[0] = "nfo:Image";
			fileTypes[1] = "nfo:RasterImage";
			break;
		case RomData::FileType::RelocatableObject:
			// TODO
			break;
		case RomData::FileType::SharedLibrary:
			fileTypes[0] = "nfo:Application";
			break;
		case RomData::FileType::CoreDump:
			// TODO
			break;
		case RomData::FileType::AudioFile:
			fileTypes[0] = "nfo:Audio";
			break;
		case RomData::FileType::BootSector:
			// TODO
			break;
		case RomData::FileType::Bundle:
			fileTypes[0] = "nfo:DataContainer";
			break;
		case RomData::FileType::ResourceFile:
			fileTypes[0] = "nfo:DataContainer";
			break;
		case RomData::FileType::Partition:
			fileTypes[0] = "nfo:FilesystemImage";
			break;

		case RomData::FileType::MetadataFile:
		case RomData::FileType::PatchFile:
		case RomData::FileType::Ticket:
			// TODO
			break;
	}

	if (fileTypes[0]) {
		switch (rp_tracker_api) {
			default:
				assert(!"Tracker API version is not supported.");
				break;

			case 1:
				tracker_sparql_pfns.v1.builder.predicate(builder, "a");
				tracker_sparql_pfns.v1.builder.object(builder, fileTypes[0]);
				if (fileTypes[1]) {
					tracker_sparql_pfns.v1.builder.object(builder, fileTypes[1]);
				}
				break;

			case 2:
			case 3:
				tracker_sparql_pfns.v2.resource.add_uri(resource, "rdf:type", fileTypes[0]);
				if (fileTypes[1]) {
					tracker_sparql_pfns.v2.resource.add_uri(resource, "rdf:type", fileTypes[1]);
				}
				break;
		}
	}

	// Process metadata properties.
	const RomMetaData *const metaData = romData->metaData();
	if (!metaData) {
		// No metadata properties. Can't do much else here.
		if (rp_tracker_api >= 2) {
			tracker_extract_pfns.v2.info.set_resource(info, resource);
		}
		return true;
	}

	switch (rp_tracker_api) {
		default:
			assert(!"Tracker API version is not supported.");
			break;
		case 1:
			add_metadata_properties_v1(metaData, builder);
			break;
		case 2:
		case 3:
			add_metadata_properties_v2(metaData, resource);
			break;
	}

	if (rp_tracker_api >= 2) {
		tracker_extract_pfns.v2.info.set_resource(info, resource);
	}
	return true;
}
