/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomMetaData.hpp: ROM metadata class.                                    *
 *                                                                         *
 * Unlike RomFields, which shows all of the information of a ROM image in  *
 * a generic list, RomMetaData stores specific properties that can be used *
 * by the desktop environment's indexer.                                   *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_ROMMETADATA_HPP__
#define __ROMPROPERTIES_LIBRPBASE_ROMMETADATA_HPP__

#include "common.h"

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <ctime>

// C++ includes.
#include <string>

namespace LibRpBase {

// Properties.
// This matches KFileMetaData::Property.
enum class Property : int {
	Invalid = -1,
	FirstProperty = 0,
	Empty = 0,

	// Audio
	BitRate,	// integer: kbit/sec
	Channels,	// integer: channels
	Duration,	// integer: duration, in milliseconds
	Genre,		// string
	SampleRate,	// integer: Hz
	TrackNumber,	// unsigned integer: track number
	ReleaseYear,	// unsigned integer: year
	Comment,	// string: comment
	Artist,		// string: artist
	Album,		// string: album
	AlbumArtist,	// string: album artist
	Composer,	// string: composer
	Lyricist,	// string: lyricist

	// Document
	Author,		// string: author
	Title,		// string: title
	Subject,	// string: subject
	Generator,	// string: application used to create this file
	PageCount,	// integer: page count
	WordCount,	// integer: word count
	LineCount,	// integer: line count
	Language,	// string: language
	Copyright,	// string: copyright
	Publisher,	// string: publisher
	CreationDate,	// timestamp: creation date
	Keywords,	// FIXME: What's the type?

	// Media
	Width,		// integer: width, in pixels
	Height,		// integer: height, in pixels
	AspectRatio,	// FIXME: Float?
	FrameRate,	// integer: number of frames per second

	// Images
	ImageMake,			// string
	ImageModel,			// string
	ImageDateTime,			// FIXME
	ImageOrientation,		// FIXME
	PhotoFlash,			// FIXME
	PhotoPixelXDimension,		// FIXME
	PhotoPixelYDimension,		// FIXME
	PhotoDateTimeOriginal,		// FIXME
	PhotoFocalLength,		// FIXME
	PhotoFocalLengthIn35mmFilm,	// FIXME
	PhotoExposureTime,		// FIXME
	PhotoFNumber,			// FIXME
	PhotoApertureValue,		// FIXME
	PhotoExposureBiasValue,		// FIXME
	PhotoWhiteBalance,		// FIXME
	PhotoMeteringMode,		// FIXME
	PhotoISOSpeedRatings,		// FIXME
	PhotoSaturation,		// FIXME
	PhotoSharpness,			// FIXME
	PhotoGpsLatitude,		// FIXME
	PhotoGpsLongitude,		// FIXME
	PhotoGpsAltitude,		// FIXME

	// Translations
	TranslationUnitsTotal,			// FIXME
	TranslationUnitsWithTranslation,	// FIXME
	TranslationUnitsWithDraftTranslation,	// FIXME
	TranslationLastAuthor,			// FIXME
	TranslationLastUpDate,			// FIXME
	TranslationTemplateDate,		// FIXME

	// Origin
	OriginUrl,		// string: origin URL
	OriginEmailSubject,	// string: subject of origin email
	OriginEmailSender,	// string: sender of origin email
	OriginEmailMessageId,	// string: message ID of origin email

	// Audio
	DiscNumber,		// integer: disc number of multi-disc set
	Location,		// string: location where audio was recorded
	Performer,		// string: (lead) performer
	Ensemble,		// string: ensemble
	Arranger,		// string: arranger
	Conductor,		// string: conductor
	Opus,			// string: opus

	// Other
	Label,			// string: label
	Compilation,		// string: compilation
	License,		// string: license information

	// TODO: More fields.
	PropertyCount,
	LastProperty = PropertyCount-1,
};

// Property types.
enum class PropertyType : uint8_t {
	FirstPropertyType = 0,
	Invalid = 0,

	Integer,		// Integer type
	UnsignedInteger,	// Unsigned integer type
	String,			// String type (UTF-8)
	Timestamp,		// UNIX timestamp

	PropertyTypeCount,
	LastPropertyType = PropertyTypeCount-1,
};

class RomMetaDataPrivate;
class RomMetaData
{
	public:
		// String format flags. (Property::String)
		// NOTE: These have the same values as RomFields::StringFormat.
		enum StringFormat {
			// Trim spaces from the end of strings.
			STRF_TRIM_END	= (1U << 3),
		};

		// ROM metadata struct.
		// Dynamically allocated.
		struct MetaData {
			Property name;		// Property name.
			PropertyType type;	// Property type.

			union _data {
				// intptr_t field to cover everything.
				// Mainly used to reset the entire field.
				intptr_t iptrvalue;

				// Integer property
				int ivalue;

				// Unsigned integer property
				unsigned int uvalue;

				// String property
				const std::string *str;

				// UNIX timestamp
				time_t timestamp;
			} data;
		};

	public:
		/**
		 * Initialize a ROM Metadata class.
		 */
		RomMetaData();
		~RomMetaData();

	private:
		RP_DISABLE_COPY(RomMetaData)
	private:
		friend class RomMetaDataPrivate;
		RomMetaDataPrivate *d_ptr;

	public:
		/** Metadata accessors. **/

		/**
		 * Get the number of metadata properties.
		 * @return Number of metadata properties.
		 */
		RP_LIBROMDATA_PUBLIC
		int count(void) const;

		/**
		 * Get a metadata property.
		 * @param idx Metadata index
		 * @return Metadata property, or nullptr if the index is invalid.
		 */
		RP_LIBROMDATA_PUBLIC
		const MetaData *prop(int idx) const;

		/**
		 * Is this RomMetaData empty?
		 * @return True if empty; false if not.
		 */
		RP_LIBROMDATA_PUBLIC
		bool empty(void) const;

	public:
		/** Convenience functions for RomData subclasses. **/

		/**
		 * Reserve space for metadata.
		 * @param n Desired capacity
		 */
		void reserve(int n);

		/**
		 * Add metadata from another RomMetaData object.
		 *
		 * If metadata properties with the same names already exist,
		 * they will be overwritten.
		 *
		 * @param other Source RomMetaData object
		 * @return Metadata index of the last metadata added.
		 */
		int addMetaData_metaData(const RomMetaData *other);

		/**
		 * Add an integer metadata property.
		 *
		 * If a metadata property with the same name already exists,
		 * it will be overwritten.
		 *
		 * @param name Property name
		 * @param val Integer value
		 * @return Metadata index, or -1 on error.
		 */
		int addMetaData_integer(Property name, int value);

		/**
		 * Add an unsigned integer metadata property.
		 * @param name Property name
		 * @param val Unsigned integer value
		 * @return Metadata index, or -1 on error.
		 */
		int addMetaData_uint(Property name, unsigned int value);

		/**
		 * Add a string metadata property.
		 *
		 * If a metadata property with the same name already exists,
		 * it will be overwritten.
		 *
		 * @param name Property name
		 * @param str String value
		 * @param flags Formatting flags
		 * @return Metadata index, or -1 on error.
		 */
		int addMetaData_string(Property name, const char *str, unsigned int flags = 0);

		/**
		 * Add a string metadata property.
		 *
		 * If a metadata property with the same name already exists,
		 * it will be overwritten.
		 *
		 * @param name Property name
		 * @param str String value
		 * @param flags Formatting flags
		 * @return Metadata index, or -1 on error.
		 */
		int addMetaData_string(Property name, const std::string &str, unsigned int flags = 0);

#ifndef CXX20_COMPAT_CHAR8_T
		/**
		 * Add a string metadata property.
		 *
		 * If a metadata property with the same name already exists,
		 * it will be overwritten.
		 *
		 * @param name Property name
		 * @param str String value
		 * @param flags Formatting flags
		 * @return Metadata index, or -1 on error.
		 */
		inline int addMetaData_string(Property name, const char8_t *str, unsigned int flags = 0)
		{
			return addMetaData_string(name, reinterpret_cast<const char*>(str), flags);
		}

		/**
		 * Add a string metadata property.
		 *
		 * If a metadata property with the same name already exists,
		 * it will be overwritten.
		 *
		 * @param name Property name
		 * @param str String value
		 * @param flags Formatting flags
		 * @return Metadata index, or -1 on error.
		 */
		inline int addMetaData_string(Property name, const std::u8string &str, unsigned int flags = 0)
		{
			return addMetaData_string(name, reinterpret_cast<const char*>(str.c_str()), flags);
		}
#endif /* CXX20_COMPAT_CHAR8_T */

		/**
		 * Add a timestamp metadata property.
		 *
		 * If a metadata property with the same name already exists,
		 * it will be overwritten.
		 *
		 * @param name Metadata name
		 * @param timestamp UNIX timestamp
		 * @return Metadata index, or -1 on error.
		 */
		int addMetaData_timestamp(Property name, time_t timestamp);
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_ROMMETADATA_HPP__ */
