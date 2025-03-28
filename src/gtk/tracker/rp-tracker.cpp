/***************************************************************************
 * ROM Properties Page shell extension. (GNOME Tracker)                    *
 * rp-tracker.cpp: Tracker extractor module                                *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "tracker-mini.h"
#include "tracker-file-utils.h"

// glib: module handling
#include <gmodule.h>

// GLib on non-Windows platforms (prior to 2.53.1) defines G_MODULE_EXPORT to a no-op.
// This doesn't work when we use symbol visibility settings.
#if !GLIB_CHECK_VERSION(2, 53, 1) && !defined(_WIN32) && (defined(__GNUC__) && __GNUC__ >= 4)
#  ifdef G_MODULE_EXPORT
#    undef G_MODULE_EXPORT
#  endif
#  define G_MODULE_EXPORT __attribute__((visibility("default")))
#endif /* !GLIB_CHECK_VERSION(2, 53, 1) && !_WIN32 && __GNUC__ >= 4 */

// libromdata
#include "librpbase/config/Config.hpp"
#include "librpbase/RomData.hpp"
#include "librpbase/RomMetaData.hpp"
#include "librpfile/FileSystem.hpp"
#include "libromdata/RomDataFactory.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRomData;

// C includes (C++ namespace)
#include <cstdlib>

// C++ STL classes
using std::array;

static void
add_metadata_properties_v1(const RomMetaData *metaData, TrackerSparqlBuilder *builder)
{
	for (const RomMetaData::MetaData &prop : *metaData) {
		if (prop.type == PropertyType::String && (!prop.data.str || prop.data.str[0] == '\0')) {
			// Should not happen...
			assert(!"nullptr string detected");
			break;
		}

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
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str);
				break;
			case Property::SampleRate:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nfo:sampleRate");
				tracker_sparql_pfns.v1.builder.object_int64(builder, prop.data.ivalue);
				break;
			case Property::TrackNumber:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nmm:trackNumber");
				tracker_sparql_pfns.v1.builder.object_int64(builder, prop.data.ivalue);
				break;

			case Property::ReleaseYear: {
				// NOTE: Converting to YYYY-01-01 00:00:00, since this is "nie:informationElementDate".
				// (equivalent to "xsd::dateTime")
				// FIXME: tracker-miners does this for MP3 Release Years, but it *might*
				// cause an off-by-one in some timezones...
				GDateTime *const dateTime = g_date_time_new_utc(
					static_cast<gint>(prop.data.uvalue), 1, 1, 0, 0, 0);
				time_t unixTime = g_date_time_to_unix(dateTime);
				g_date_time_unref(dateTime);

				tracker_sparql_pfns.v1.builder.predicate(builder, "nmm:releaseDate");
				tracker_sparql_pfns.v1.builder.object_date(builder, &unixTime);
				break;
			}

			case Property::Artist:
				// TODO
				break;
			case Property::Album:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nmm:musicAlbum");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str);
				break;
			case Property::AlbumArtist:
				// TODO
				break;
			case Property::Composer:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nmm:composer");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str);
				break;
			case Property::Lyricist:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nmm:lyricist");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str);
				break;

			// Document
			case Property::Author:
				// NOTE: Closest equivalent is "Creator".
				tracker_sparql_pfns.v1.builder.predicate(builder, "nco:creator");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str);
				break;
			case Property::Title:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nie:title");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str);
				break;
			case Property::Copyright:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nie:copyright");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str);
				break;
			case Property::Publisher:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nco:publisher");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str);
				break;
			case Property::Description:
				tracker_sparql_pfns.v1.builder.predicate(builder, "nie:description");
				tracker_sparql_pfns.v1.builder.object_string(builder, prop.data.str);
				break;
			case Property::CreationDate: {
				tracker_sparql_pfns.v1.builder.predicate(builder, "nie:contentCreated");
				time_t unixTime = prop.data.timestamp;
				tracker_sparql_pfns.v1.builder.object_date(builder, &unixTime);
				break;
			}

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
	// for album relations
	TrackerResource *album_artist = nullptr;
	const char *album_name = nullptr;
	int disc_number = 0;
	bool has_disc_number = false;

	for (const RomMetaData::MetaData &prop : *metaData) {
		if (prop.type == PropertyType::String && !prop.data.str) {
			// Should not happen...
			assert(!"nullptr string detected");
			break;
		}

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
				tracker_sparql_pfns.v2.resource.set_string(resource, "nmm:genre", prop.data.str);
				break;
			case Property::SampleRate:
				tracker_sparql_pfns.v2.resource.set_int(resource, "nfo:sampleRate", prop.data.ivalue);
				break;
			case Property::TrackNumber:
				tracker_sparql_pfns.v2.resource.set_int(resource, "nmm:trackNumber", prop.data.ivalue);
				break;

			case Property::ReleaseYear: {
				// NOTE: Converting to YYYY-01-01 00:00:00, since this is "nie:informationElementDate".
				// (equivalent to "xsd::dateTime")
				// FIXME: tracker-miners does this for MP3 Release Years, but it *might*
				// cause an off-by-one in some timezones...
				GDateTime *const dateTime = g_date_time_new_utc(
					static_cast<gint>(prop.data.uvalue), 1, 1, 0, 0, 0);

				GValue value = G_VALUE_INIT;
				g_value_init(&value, G_TYPE_DATE_TIME);
				g_value_set_boxed(&value, dateTime);
				g_date_time_unref(dateTime);

				tracker_sparql_pfns.v2.resource.set_gvalue(resource, "nmm:releaseDate", &value);

				g_value_unset(&value);
				break;
			}

			case Property::Artist:
				// TODO
				break;
			case Property::Album:
				// NOTE: Property is added later.
				//tracker_sparql_pfns.v2.resource.set_string(resource, "nmm:musicAlbum", prop.data.str);
				album_name = prop.data.str;
				break;
			case Property::AlbumArtist:
				// NOTE: Property is added later. (as part of Album, or standalone if not specified)
				// TODO: Separate from composer?
				//tracker_sparql_pfns.v2.resource.set_string(resource, "nmm:composer", prop.data.str);
				album_artist = tracker_extract_pfns.v2._new.artist(prop.data.str);
				break;
			case Property::Composer: {
				TrackerResource *const composer = tracker_extract_pfns.v2._new.artist(prop.data.str);
				tracker_sparql_pfns.v2.resource.add_take_relation(resource, "nmm:composer", composer);
				break;
			}
			case Property::Lyricist: {
				TrackerResource *const lyricist = tracker_extract_pfns.v2._new.artist(prop.data.str);
				tracker_sparql_pfns.v2.resource.add_take_relation(resource, "nmm:lyricist", lyricist);
				break;
			}

			// Document
			case Property::Author:
				// NOTE: Closest equivalent is "Creator".
				tracker_sparql_pfns.v2.resource.set_string(resource, "nco:creator", prop.data.str);
				break;
			case Property::Title:
				tracker_sparql_pfns.v2.resource.set_string(resource, "nie:title", prop.data.str);
				break;
			case Property::Copyright:
				tracker_sparql_pfns.v2.resource.set_string(resource, "nie:copyright", prop.data.str);
				break;
			case Property::Publisher:
				tracker_sparql_pfns.v2.resource.set_string(resource, "nco:publisher", prop.data.str);
				break;
			case Property::Description:
				tracker_sparql_pfns.v2.resource.set_string(resource, "nie:description", prop.data.str);
				break;

			case Property::CreationDate: {
				// TODO: Store UTC or Local flag? Or not, since Tracker expects UTC...
				GDateTime *const dateTime = g_date_time_new_from_unix_utc(prop.data.timestamp);

				GValue value = G_VALUE_INIT;
				g_value_init(&value, G_TYPE_DATE_TIME);
				g_value_set_boxed(&value, dateTime);
				g_date_time_unref(dateTime);

				// FIXME: "nie:created" or "nie:contentCreated"?
				tracker_sparql_pfns.v2.resource.set_gvalue(resource, "nie:contentCreated", &value);

				g_value_unset(&value);
				break;
			}

			// Media
			case Property::Width:
				tracker_sparql_pfns.v2.resource.set_int(resource, "nfo:width", prop.data.ivalue);
				break;
			case Property::Height:
				tracker_sparql_pfns.v2.resource.set_int(resource, "nfo:height", prop.data.ivalue);
				break;

			// Audio
			case Property::DiscNumber:
				// NOTE: Property is added later. (as part of Album, or standalone if not specified)
				disc_number = prop.data.ivalue;
				has_disc_number = true;
				break;
		}
	}

	if (album_name) {
		// Create an Album relation.
		// TODO: Release date
		TrackerResource *const album_disc = tracker_extract_pfns.v2._new.music_album_disc(
			album_name, album_artist, (disc_number > 0) ? disc_number : 1, "");

		tracker_sparql_pfns.v2.resource.set_take_relation(resource, "nmm:musicAlbumDisc", album_disc);

		TrackerResource *const album = tracker_sparql_pfns.v2.resource.get_first_relation(album_disc, "nmm:albumDiscAlbum");
		tracker_sparql_pfns.v2.resource.set_relation(resource, "nmm:musicAlbum", album);
	} else {
		// Set other properties.
		if (has_disc_number) {
			tracker_sparql_pfns.v2.resource.set_int(resource, "nmm:setNumber", disc_number);
		}

		// TODO: album_artist?
	}

	g_clear_object(&album_artist);
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

	gchar *const filename = g_file_get_path(file);
	if (!filename) {
		// Unable to get a local filename.
		// TODO: Support URIs?
		return false;
	}

	// Check for "bad" file systems.
	Config *const config = Config::instance();
	if (FileSystem::isOnBadFS(filename, config->getBoolConfigOption(Config::BoolConfig::Options_EnableThumbnailOnNetworkFS))) {
		// This file is on a "bad" file system.
		g_free(filename);
		return false;
	}

	// Attempt to open the file using RomDataFactory.
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
			gchar *const resource_uri = tracker_file_get_content_identifier(file, nullptr, nullptr);
			resource = tracker_sparql_pfns.v2.resource._new(resource_uri);
			g_free(resource_uri);
			break;
		}
	}

	// Determine the file type.
	// TODO: Better NFOs for some of these.
	array<const char*, 2> fileTypes = {{nullptr, nullptr}};
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

/** Shutdown function **/

// NOTE: The init function had a TrackerModuleThreadAwareness parameter
// prior to 1.11.2. We can't tell if it's v1 or v2+ at this point,
// so we'll only implement the shutdown() function.

extern "C"
G_MODULE_EXPORT void
tracker_extract_module_shutdown(void)
{
	rp_tracker_free_pfn();
}
