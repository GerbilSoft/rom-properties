/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomMetaData.cpp: ROM metadata class.                                    *
 *                                                                         *
 * Unlike RomFields, which shows all of the information of a ROM image in  *
 * a generic list, RomMetaData stores specific properties that can be used *
 * by the desktop environment's indexer.                                   *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "RomMetaData.hpp"

#include "common.h"
#include "TextFuncs.hpp"
#include "threads/Atomics.h"
#include "libi18n/i18n.h"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <limits>
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRpBase {

class RomMetaDataPrivate
{
	public:
		RomMetaDataPrivate();
		~RomMetaDataPrivate();

	private:
		RP_DISABLE_COPY(RomMetaDataPrivate)

	public:
		// ROM field structs.
		vector<RomMetaData::MetaData> metaData;

		/**
		 * Deletes allocated strings in this->metaData.
		 */
		void delete_data(void);

		// Property type mapping.
		static const uint8_t PropertyTypeMap[];
};

/** RomMetaDataPrivate **/

// Property type mapping.
const uint8_t RomMetaDataPrivate::PropertyTypeMap[] = {
	PropertyType::FirstPropertyType,	// first type is invalid

	// Audio
	PropertyType::Integer,	// BitRate
	PropertyType::Integer,	// Channels
	PropertyType::Integer,	// Duration
	PropertyType::String,	// Genre
	PropertyType::Integer,	// Sample Rate
	PropertyType::UnsignedInteger,	// Track number
	PropertyType::UnsignedInteger,	// Release Year
	PropertyType::String,	// Comment
	PropertyType::String,	// Artist
	PropertyType::String,	// Album
	PropertyType::String,	// AlbumArtist
	PropertyType::String,	// Composer
	PropertyType::String,	// Lyricist

	// Document
	PropertyType::String,	// Author
	PropertyType::String,	// Title
	PropertyType::String,	// Subject
	PropertyType::String,	// Generator
	PropertyType::Integer,	// PageCount
	PropertyType::Integer,	// WordCount
	PropertyType::Integer,	// LineCount
	PropertyType::String,	// Language
	PropertyType::String,	// Copyright
	PropertyType::String,	// Publisher
	PropertyType::Timestamp, // CreationDate
	PropertyType::Invalid,	// Keywords (FIXME)

	// Media
	PropertyType::Integer,	// Width
	PropertyType::Integer,	// Height
	PropertyType::Invalid,	// AspectRatio (FIXME: Float?)
	PropertyType::Integer,	// FrameRate

	// Images
	PropertyType::String,	// ImageMake
	PropertyType::String,	// ImageModel
	PropertyType::Timestamp, // ImageDateTime
	PropertyType::Invalid,	// ImageOrientation (FIXME)
	PropertyType::Invalid,	// PhotoFlash (FIXME)
	PropertyType::Invalid,	// PhotoPixelXDimension (FIXME)
	PropertyType::Invalid,	// PhotoPixelYDimension (FIXME)
	PropertyType::Timestamp, // PhotoDateTimeOriginal
	PropertyType::Invalid,	// PhotoFocalLength (FIXME)
	PropertyType::Invalid,	// PhotoFocalLengthIn35mmFilm (FIXME)
	PropertyType::Invalid,	// PhotoExposureTime (FIXME)
	PropertyType::Invalid,	// PhotoFNumber (FIXME)
	PropertyType::Invalid,	// PhotoApertureValue (FIXME)
	PropertyType::Invalid,	// PhotoExposureBiasValue (FIXME)
	PropertyType::Invalid,	// PhotoWhiteBalance (FIXME)
	PropertyType::Invalid,	// PhotoMeteringMode (FIXME)
	PropertyType::Invalid,	// PhotoISOSpeedRatings (FIXME)
	PropertyType::Invalid,	// PhotoSaturation (FIXME)
	PropertyType::Invalid,	// PhotoSharpness (FIXME)
	PropertyType::Invalid,	// PhotoGpsLatitude (FIXME)
	PropertyType::Invalid,	// PhotoGpsLongitude (FIXME)
	PropertyType::Invalid,	// PhotoGpsAltitude (FIXME)

	// Translations
	PropertyType::Invalid,	// TranslationUnitsTotal (FIXME)
	PropertyType::Invalid,	// TranslationUnitsWithTranslation (FIXME)
	PropertyType::Invalid,	// TranslationUnitsWithDraftTranslation (FIXME)
	PropertyType::Invalid,	// TranslationLastAuthor (FIXME)
	PropertyType::Invalid,	// TranslationLastUpDate (FIXME)
	PropertyType::Invalid,	// TranslationTemplateDate (FIXME)

	// Origin
	PropertyType::String,	// OriginUrl
	PropertyType::String,	// OriginEmailSubject
	PropertyType::String,	// OriginEmailSender
	PropertyType::String,	// OriginEmailMessageId

	// Audio
	PropertyType::UnsignedInteger,	// DiscNumber [TODO verify unsigned]
	PropertyType::String,	// Location
	PropertyType::String,	// Performer
	PropertyType::String,	// Ensemble
	PropertyType::String,	// Arranger
	PropertyType::String,	// Conductor
	PropertyType::String,	// Opus

	// Other
	PropertyType::String,	// Label
	PropertyType::String,	// Compilation
	PropertyType::String,	// License
};

RomMetaDataPrivate::RomMetaDataPrivate()
{
	static_assert(ARRAY_SIZE(RomMetaDataPrivate::PropertyTypeMap) == Property::PropertyCount,
		      "PropertyTypeMap needs to be updated!");
}

RomMetaDataPrivate::~RomMetaDataPrivate()
{
	delete_data();
}

/**
 * Deletes allocated strings in this->data.
 */
void RomMetaDataPrivate::delete_data(void)
{
	// Delete all of the allocated strings in this->metaData.
	for (int i = static_cast<int>(metaData.size() - 1); i >= 0; i--) {
		RomMetaData::MetaData &metaData = this->metaData.at(i);

		assert(metaData.name > Property::FirstProperty);
		assert(metaData.name < Property::PropertyCount);
		if (metaData.name <= Property::FirstProperty ||
		    metaData.name >= Property::PropertyCount)
		{
			continue;
		}

		switch (PropertyTypeMap[metaData.name]) {
			case PropertyType::Integer:
			case PropertyType::UnsignedInteger:
			case PropertyType::Timestamp:
				// No data here.
				break;
			case PropertyType::String:
				delete const_cast<string*>(metaData.data.str);
				break;
			default:
				// ERROR!
				assert(!"Unsupported RomMetaData PropertyType.");
				break;
		}
	}

	// Clear the metadata vector.
	this->metaData.clear();
}

/** RomMetaData **/

/**
 * Initialize a ROM Fields class.
 * @param fields Array of metaData.
 * @param count Number of metaData.
 */
RomMetaData::RomMetaData()
	: d_ptr(new RomMetaDataPrivate())
{ }

RomMetaData::~RomMetaData()
{
	delete d_ptr;
}

/** Metadata accessors. **/

/**
 * Get the number of metadata properties.
 * @return Number of metadata properties.
 */
int RomMetaData::count(void) const
{
	RP_D(const RomMetaData);
	return static_cast<int>(d->metaData.size());
}

/**
 * Get a metadata property.
 * @param idx Metadata index.
 * @return Metadata property, or nullptr if the index is invalid.
 */
const RomMetaData::MetaData *RomMetaData::prop(int idx) const
{
	RP_D(const RomMetaData);
	if (idx < 0 || idx >= static_cast<int>(d->metaData.size()))
		return nullptr;
	return &d->metaData[idx];
}

/**
 * Is this RomMetaData empty?
 * @return True if empty; false if not.
 */
bool RomMetaData::empty(void) const
{
	RP_D(const RomMetaData);
	return d->metaData.empty();
}

/** Convenience functions for RomData subclasses. **/

/**
 * Reserve space for metadata.
 * @param n Desired capacity.
 */
void RomMetaData::reserve(int n)
{
	assert(n > 0);
	if (n > 0) {
		RP_D(RomMetaData);
		d->metaData.reserve(n);
	}
}

/**
 * Add metadata from another RomMetaData object.
 * @param other Source RomMetaData object.
 * @return Metadata index of the last metadata added.
 */
int RomMetaData::addMetaData_metaData(const RomMetaData *other)
{
	RP_D(RomMetaData);

	assert(other != nullptr);
	if (!other)
		return -1;

	// TODO: More tab options:
	// - Add original tab names if present.
	// - Add all to specified tab or to current tab.
	// - Use absolute or relative tab offset.
	d->metaData.reserve(d->metaData.size() + other->count());

	for (int i = 0; i < other->count(); i++) {
		const MetaData *src = other->prop(i);
		if (!src)
			continue;

		size_t idx = d->metaData.size();
		d->metaData.resize(idx+1);
		MetaData &metaData = d->metaData.at(idx);
		metaData.name = src->name;

		assert(metaData.name > Property::FirstProperty);
		assert(metaData.name < Property::PropertyCount);
		if (metaData.name <= Property::FirstProperty ||
		    metaData.name >= Property::PropertyCount)
		{
			continue;
		}

		metaData.type = src->type;
		switch (metaData.type) {
			case PropertyType::Integer:
				metaData.data.ivalue = src->data.ivalue;
				break;
			case PropertyType::UnsignedInteger:
				metaData.data.uvalue = src->data.uvalue;
				break;
			case PropertyType::String:
				metaData.data.str = new string(*src->data.str);
				break;
			case PropertyType::Timestamp:
				metaData.data.timestamp = src->data.timestamp;
				break;
			default:
				// ERROR!
				assert(!"Unsupported RomMetaData PropertyType.");
				break;
		}
	}

	// Fields added.
	return static_cast<int>(d->metaData.size() - 1);
}

/**
 * Add an integer metadata property.
 * @param name Metadata name.
 * @param val Integer value.
 * @return Metadata index.
 */
int RomMetaData::addMetaData_integer(Property::Property name, int value)
{
	assert(name > Property::FirstProperty);
	assert(name < Property::PropertyCount);
	if (name <= Property::FirstProperty ||
	    name >= Property::PropertyCount)
	{
		return -1;
	}

	// Make sure this is an integer property.
	assert(RomMetaDataPrivate::PropertyTypeMap[name] == PropertyType::Integer);
	if (RomMetaDataPrivate::PropertyTypeMap[name] != PropertyType::Integer)
		return -1;

	RP_D(RomMetaData);
	size_t idx = d->metaData.size();
	d->metaData.resize(idx+1);
	MetaData &metaData = d->metaData.at(idx);

	metaData.name = name;
	metaData.type = PropertyType::Integer;
	metaData.data.ivalue = value;
	return static_cast<int>(idx);
}

/**
 * Add an unsigned integer metadata property.
 * @param name Metadata name.
 * @param val Unsigned integer value.
 * @return Metadata index.
 */
int RomMetaData::addMetaData_uint(Property::Property name, unsigned int value)
{
	assert(name > Property::FirstProperty);
	assert(name < Property::PropertyCount);
	if (name <= Property::FirstProperty ||
	    name >= Property::PropertyCount)
	{
		return -1;
	}

	// Make sure this is an integer property.
	assert(RomMetaDataPrivate::PropertyTypeMap[name] == PropertyType::UnsignedInteger);
	if (RomMetaDataPrivate::PropertyTypeMap[name] != PropertyType::UnsignedInteger)
		return -1;

	RP_D(RomMetaData);
	size_t idx = d->metaData.size();
	d->metaData.resize(idx+1);
	MetaData &metaData = d->metaData.at(idx);

	metaData.name = name;
	metaData.type = PropertyType::UnsignedInteger;
	metaData.data.uvalue = value;
	return static_cast<int>(idx);
}

/**
 * Add a string metadata property.
 * @param name Metadata name.
 * @param str String value.
 * @param flags Formatting flags.
 * @return Metadata index.
 */
int RomMetaData::addMetaData_string(Property::Property name, const char *str, unsigned int flags)
{
	assert(name > Property::FirstProperty);
	assert(name < Property::PropertyCount);
	if (name <= Property::FirstProperty || name >= Property::PropertyCount)
		return -1;

	// Make sure this is an integer property.
	assert(RomMetaDataPrivate::PropertyTypeMap[name] == PropertyType::String);
	if (RomMetaDataPrivate::PropertyTypeMap[name] != PropertyType::String)
		return -1;

	RP_D(RomMetaData);
	size_t idx = d->metaData.size();
	d->metaData.resize(idx+1);
	MetaData &metaData = d->metaData.at(idx);

	string *const nstr = (str ? new string(str) : nullptr);
	metaData.name = name;
	metaData.type = PropertyType::String;
	metaData.data.str = nstr;

	// Trim the string if requested.
	if (nstr && (flags & STRF_TRIM_END)) {
		trimEnd(*nstr);
	}
	return static_cast<int>(idx);
}

/**
 * Add a string metadata property.
 * @param name Metadata name.
 * @param str String value.
 * @param flags Formatting flags.
 * @return Metadata index.
 */
int RomMetaData::addMetaData_string(Property::Property name, const string &str, unsigned int flags)
{
	assert(name > Property::FirstProperty);
	assert(name < Property::PropertyCount);
	if (name <= Property::FirstProperty || name >= Property::PropertyCount)
		return -1;

	// Make sure this is an integer property.
	assert(RomMetaDataPrivate::PropertyTypeMap[name] == PropertyType::String);
	if (RomMetaDataPrivate::PropertyTypeMap[name] != PropertyType::String)
		return -1;

	RP_D(RomMetaData);
	size_t idx = d->metaData.size();
	d->metaData.resize(idx+1);
	MetaData &metaData = d->metaData.at(idx);

	string *const nstr = (!str.empty() ? new string(str) : nullptr);
	metaData.name = name;
	metaData.type = PropertyType::String;
	metaData.data.str = nstr;

	// Trim the string if requested.
	if (nstr && (flags & STRF_TRIM_END)) {
		trimEnd(*nstr);
	}
	return static_cast<int>(idx);
}

/**
 * Add a timestamp metadata property.
 * @param name Metadata name.
 * @param timestamp UNIX timestamp.
 * @return Metadata index.
 */
int RomMetaData::addMetaData_timestamp(Property::Property name, time_t timestamp)
{
	assert(name > Property::FirstProperty);
	assert(name < Property::PropertyCount);
	if (name <= Property::FirstProperty || name >= Property::PropertyCount)
		return -1;

	// Make sure this is a timestamp property.
	assert(RomMetaDataPrivate::PropertyTypeMap[name] == PropertyType::Timestamp);
	if (RomMetaDataPrivate::PropertyTypeMap[name] != PropertyType::Timestamp)
		return -1;

	RP_D(RomMetaData);
	size_t idx = d->metaData.size();
	d->metaData.resize(idx+1);
	MetaData &metaData = d->metaData.at(idx);

	metaData.name = name;
	metaData.type = PropertyType::Timestamp;
	metaData.data.timestamp = timestamp;
	return static_cast<int>(idx);
}

}
