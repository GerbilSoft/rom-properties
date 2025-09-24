/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomMetaData.hpp: ROM metadata class.                                    *
 *                                                                         *
 * Unlike RomFields, which shows all of the information of a ROM image in  *
 * a generic list, RomMetaData stores specific properties that can be used *
 * by the desktop environment's indexer.                                   *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

// C includes (C++ namespace)
#include <cstdint>
#include <ctime>

// C++ includes
#include <string>

namespace LibRpBase {

// Properties.
// Most of the lower properties match KFileMetaData::Property, but a lot
// of the later KFileMetaData properties aren't useful for us, so we're
// using a custom mapping.
enum class Property : int8_t {
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
	Manufacturer,		// string
	Model,			// string
	ImageDateTime,		// FIXME
	ImageOrientation,	// FIXME
	PhotoFlash,		// FIXME

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

	// Added in KF5 5.48
	Rating,			// integer: [0,100]
	Lyrics,			// string

	// Added in KF5 5.53
	Description,		// string

	/** Custom properties! **/
	GameID,			// string
	TitleID,		// string
	MediaID,		// string
	OSVersion,		// string
	EncryptionKey,		// string
	PixelFormat,		// string
	Region,			// string

	// TODO: More fields.
	PropertyCount,
	LastProperty = PropertyCount-1,
	LastKFMDProperty = Description,
};

// Property types.
enum class PropertyType : uint8_t {
	FirstPropertyType = 0,
	Invalid = 0,

	Integer,		// Integer type
	UnsignedInteger,	// Unsigned integer type
	String,			// String type (UTF-8)
	Timestamp,		// UNIX timestamp
	Double,			// Double-precision floating point

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

		/**
		 * Initialize a RomMetaData::MetaData object.
		 * Defaults to zero init.
		 */
		MetaData();

		/**
		 * Initialize a RomMetaData::MetaData object.
		 * Property data will be zero-initialized.
		 * @param name
		 * @param type
		 */
		MetaData(Property name, PropertyType type);

		// Destructor to handle automatic string deletion.
		~MetaData();

		MetaData(const MetaData &other);		// copy constructor
		MetaData& operator=(MetaData other);		// assignment operator
		MetaData(MetaData &&other) noexcept;		// move constructor
		MetaData& operator=(MetaData &&other) noexcept;	// move assignment operator

		/** Fields **/

		union _data {
			// intptr_t field to cover everything.
			// Mainly used to reset the entire field.
			intptr_t iptrvalue;

			// Integer property
			int ivalue;

			// Unsigned integer property
			unsigned int uvalue;

			// String property
			const char *str;

			// UNIX timestamp
			time_t timestamp;

			// Double-precision floating point value
			double dvalue;
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
	/** Metadata iterator types **/
	typedef std::vector<MetaData>::const_iterator const_iterator;

public:
	/** Metadata accessors **/

	/**
	 * Get the number of metadata properties.
	 * @return Number of metadata properties.
	 */
	RP_LIBROMDATA_PUBLIC
	int count(void) const;

	/**
	 * Is this RomMetaData empty?
	 * @return True if empty; false if not.
	 */
	RP_LIBROMDATA_PUBLIC
	bool empty(void) const;

	/**
	 * Get a metadata property, by index.
	 * @param idx Metadata index
	 * @return Metadata property, or nullptr if the index is invalid.
	 */
	RP_LIBROMDATA_PUBLIC
	const MetaData *at(int idx) const;

	/**
	 * Get a metadata property, by name.
	 * @param name Metadata property name
	 * @return Metadata property, or nullptr if the property isn't set.
	 */
	RP_LIBROMDATA_PUBLIC
	const MetaData *get(Property name) const;

	/**
	 * Get a const iterator pointing to the beginning of the RomMetaData.
	 * @return Const iterator
	 */
	RP_LIBROMDATA_PUBLIC
	const_iterator cbegin(void) const;

	/**
	 * Get a const iterator pointing to the end of the RomMetaData.
	 * @return Const iterator
	 */
	RP_LIBROMDATA_PUBLIC
	const_iterator cend(void) const;

	/**
	 * Get a const iterator pointing to the beginning of the RomMetaData.
	 * Alias function required for range-based `for` loops.
	 * @return Const iterator
	 */
	const_iterator begin(void) const
	{
		return cbegin();
	}

	/**
	 * Get a const iterator pointing to the end of the RomMetaData.
	 * Alias function required for range-based `for` loops.
	 * @return Const iterator
	 */
	const_iterator end(void) const
	{
		return cend();
	}

public:
	/** Convenience functions for RomData subclasses **/

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
	int addMetaData_string(Property name, const std::string &str, unsigned int flags = 0)
	{
		if (str.empty()) {
			// Ignore empty strings.
			return -1;
		}
		return addMetaData_string(name, str.c_str(), flags);
	}

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

	/**
	 * Add a double-precision floating point metadata property.
	 *
	 * If a metadata property with the same name already exists,
	 * it will be overwritten.
	 *
	 * @param name Property name
	 * @param dvalue Double value
	 * @return Metadata index, or -1 on error.
	 */
	int addMetaData_double(Property name, double dvalue);
};

} // namespace LibRpBase
