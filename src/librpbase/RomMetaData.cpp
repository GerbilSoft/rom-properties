/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomMetaData.cpp: ROM metadata class.                                    *
 *                                                                         *
 * Unlike RomFields, which shows all of the information of a ROM image in  *
 * a generic list, RomMetaData stores specific properties that can be used *
 * by the desktop environment's indexer.                                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RomMetaData.hpp"

// Other rom-properties libraries
using namespace LibRpText;

// C++ STL classes
using std::string;
using std::vector;

namespace LibRpBase {

class RomMetaDataPrivate
{
	public:
		RomMetaDataPrivate();

	private:
		RP_DISABLE_COPY(RomMetaDataPrivate)

	public:
		// ROM field structs.
		vector<RomMetaData::MetaData> metaData;

		// Mapping of Property to metaData indexes.
		// Index == Property
		// Value == metaData index (-1 for none)
		std::array<Property, (int)Property::PropertyCount> map_metaData;

		// Property type mapping
		static const PropertyType PropertyTypeMap[];

		/**
		 * Add or overwrite a Property.
		 * @param name Property name
		 * @return Metadata property
		 */
		RomMetaData::MetaData *addProperty(Property name);
};

/** RomMetaDataPrivate **/

// Property type mapping
const PropertyType RomMetaDataPrivate::PropertyTypeMap[] = {
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

	// Added in KF5 5.48
	PropertyType::Integer,	// Rating
	PropertyType::String,	// Lyrics

	// Replay gain (KF5 5.51)
	PropertyType::Double,	// ReplayGainAlbumPeak
	PropertyType::Double,	// ReplayGainAlbumGain
	PropertyType::Double,	// ReplayGainTrackPeak
	PropertyType::Double,	// ReplayGainTrackGain

	// Added in KF5 5.53
	PropertyType::String,	// Description
};

RomMetaDataPrivate::RomMetaDataPrivate()
{
	static_assert(ARRAY_SIZE(RomMetaDataPrivate::PropertyTypeMap) == (int)Property::PropertyCount,
		      "PropertyTypeMap needs to be updated!");
	map_metaData.fill(Property::Invalid);
}

/**
 * Add or overwrite a Property.
 * @param name Property name
 * @return Metadata property
 */
RomMetaData::MetaData *RomMetaDataPrivate::addProperty(Property name)
{
	assert(name > Property::FirstProperty);
	assert(name < Property::PropertyCount);
	if (name <= Property::FirstProperty ||
	    name >= Property::PropertyCount)
	{
		return nullptr;
	}

	// Check if this metadata property was already added.
	RomMetaData::MetaData *pMetaData;
	if (map_metaData[(int)name] > Property::Invalid) {
		// Already added. Overwrite it.
		pMetaData = &metaData[(int)map_metaData[(int)name]];
		// If a string is present, delete it.
		if (pMetaData->type == PropertyType::String) {
			delete pMetaData->data.str;
			pMetaData->data.str = nullptr;
		}
	} else {
		// Not added yet. Create a new one.
		assert(metaData.size() < 128);
		if (metaData.size() >= 128) {
			// Can't add any more properties...
			return nullptr;
		}

		metaData.emplace_back(name, static_cast<PropertyType>(PropertyTypeMap[static_cast<int>(name)]));
		size_t idx = metaData.size() - 1;
		pMetaData = &metaData[idx];
		map_metaData[static_cast<int>(name)] = static_cast<Property>(idx);
	}

	return pMetaData;
}

/** RomMetaData::MetaData **/

/**
 * Initialize a RomMetaData::MetaData object.
 * Defaults to zero init.
 */
RomMetaData::MetaData::MetaData()
	: name(Property::Invalid)
	, type(PropertyType::Invalid)
{
	data.iptrvalue = 0;
}

/**
* Initialize a RomMetaData::MetaData object.
* Property data will be zero-initialized.
* @param name
* @param type
*/
RomMetaData::MetaData::MetaData(Property name, PropertyType type)
	: name(name)
	, type(type)
{
	data.iptrvalue = 0;
}

RomMetaData::MetaData::~MetaData()
{
	// Ensure allocated data values get deleted.
	switch (this->type) {
		default:
			// ERROR!
			assert(!"Unsupported RomMetaData PropertyType.");
			break;

		case PropertyType::Invalid:
			// Destroying an invalid property.
			// May have been the source object for std::move,
			// so we'll allow it.
			break;

		case PropertyType::Integer:
		case PropertyType::UnsignedInteger:
		case PropertyType::Timestamp:
		case PropertyType::Double:
			// No allocated data here.
			break;

		case PropertyType::String:
			delete const_cast<string*>(this->data.str);
			break;
	}
}

/**
 * Copy constructor
 *
 * NOTE: This only ensures that the string data is copied correctly.
 * It will not handle map index updates.
 *
 * @param other Other RomMetaData::MetaData object
 */
RomMetaData::MetaData::MetaData(const MetaData &other)
{
	assert(other.name != Property::Invalid);
	this->name = other.name;
	this->type = other.type;

	switch (other.type) {
		default:
			// ERROR!
			assert(!"Unsupported RomMetaData PropertyType.");
			this->data.iptrvalue = 0;
			break;

		case PropertyType::Integer:
			this->data.ivalue = other.data.ivalue;
			break;
		case PropertyType::UnsignedInteger:
			this->data.uvalue = other.data.uvalue;
			break;
		case PropertyType::Timestamp:
			this->data.timestamp = other.data.timestamp;
			break;
		case PropertyType::Double:
			this->data.dvalue = other.data.dvalue;
			break;

		case PropertyType::String:
			this->data.str = (other.data.str)
				? new string(*other.data.str)
				: nullptr;
			break;
	}
}

/**
 * Assignment operator
 *
 * NOTE: This only ensures that the string data is copied correctly.
 * It will not handle map index updates.
 *
 * @param other Other RomMetaData::MetaData object
 */
RomMetaData::MetaData& RomMetaData::MetaData::operator=(MetaData other)
{
	// Swap the objects.
	// References:
	// - https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
	// - https://stackoverflow.com/a/3279550
	std::swap(*this, other);
	return *this;
}

/**
 * Move constructor
 *
 * NOTE: This only ensures that the string data is copied correctly.
 * It will not handle map index updates.
 *
 * @param other Other RomMetaData::MetaData object
 */
RomMetaData::MetaData::MetaData(MetaData &&other) noexcept
	: name(other.name)
	, type(other.type)
{
	// Copy data, then reset the other MetaData object.
	memcpy(&this->data, &other.data, sizeof(this->data));
	other.name = Property::Invalid;
	other.type = PropertyType::Invalid;
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
 * @param idx Metadata index
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
 * @param n Desired capacity
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
 *
 * If metadata properties with the same names already exist,
 * they will be overwritten.
 *
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
		const MetaData *const pSrc = other->prop(i);
		if (!pSrc)
			continue;

		assert(pSrc->name > Property::FirstProperty);
		assert(pSrc->name < Property::PropertyCount);
		if (pSrc->name <= Property::FirstProperty ||
		    pSrc->name >= Property::PropertyCount)
		{
			continue;
		}

		MetaData *const pDest = d->addProperty(pSrc->name);
		assert(pDest != nullptr);
		if (!pDest)
			break;
		assert(pDest->type == pSrc->type);
		if (pDest->type != pSrc->type)
			continue;	// TODO: Should be break?

		switch (pSrc->type) {
			case PropertyType::Integer:
				pDest->data.ivalue = pSrc->data.ivalue;
				break;
			case PropertyType::UnsignedInteger:
				pDest->data.uvalue = pSrc->data.uvalue;
				break;
			case PropertyType::String:
				// TODO: Don't add a property if the string value is nullptr?
				assert(pSrc->data.str != nullptr);
				pDest->data.str = (pSrc->data.str ? new string(*pSrc->data.str) : nullptr);
				break;
			case PropertyType::Timestamp:
				pDest->data.timestamp = pSrc->data.timestamp;
				break;
			case PropertyType::Double:
				pDest->data.dvalue = pSrc->data.dvalue;
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
 *
 * If a metadata property with the same name already exists,
 * it will be overwritten.
 *
 * @param name Property name
 * @param val Integer value
 * @return Metadata index, or -1 on error.
 */
int RomMetaData::addMetaData_integer(Property name, int value)
{
	RP_D(RomMetaData);
	MetaData *const pMetaData = d->addProperty(name);
	assert(pMetaData != nullptr);
	if (!pMetaData)
		return -1;

	// Make sure this is an integer property.
	assert(pMetaData->type == PropertyType::Integer);
	if (pMetaData->type != PropertyType::Integer) {
		// TODO: Delete the property in this case?
		pMetaData->data.iptrvalue = 0;
		return -1;
	}

	pMetaData->data.ivalue = value;
	return static_cast<int>(d->map_metaData[(int)name]);
}

/**
 * Add an unsigned integer metadata property.
 *
 * If a metadata property with the same name already exists,
 * it will be overwritten.
 *
 * @param name Property name
 * @param val Unsigned integer value
 * @return Metadata index, or -1 on error.
 */
int RomMetaData::addMetaData_uint(Property name, unsigned int value)
{
	RP_D(RomMetaData);
	MetaData *const pMetaData = d->addProperty(name);
	assert(pMetaData != nullptr);
	if (!pMetaData)
		return -1;

	// Make sure this is an unsigned integer property.
	assert(pMetaData->type == PropertyType::UnsignedInteger);
	if (pMetaData->type != PropertyType::UnsignedInteger) {
		// TODO: Delete the property in this case?
		pMetaData->data.iptrvalue = 0;
		return -1;
	}

	pMetaData->data.uvalue = value;
	return static_cast<int>(d->map_metaData[(int)name]);
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
int RomMetaData::addMetaData_string(Property name, const char *str, unsigned int flags)
{
	if (!str || str[0] == '\0') {
		// Ignore empty strings.
		return -1;
	}

	string *const nstr = new string(str);
	// Trim the string if requested.
	if (nstr && (flags & STRF_TRIM_END)) {
		trimEnd(*nstr);
	}
	if (nstr->empty()) {
		// String is now empty. Ignore it.
		delete nstr;
		return -1;
	}

	RP_D(RomMetaData);
	MetaData *const pMetaData = d->addProperty(name);
	assert(pMetaData != nullptr);
	if (!pMetaData)
		return -1;

	// Make sure this is a string property.
	assert(pMetaData->type == PropertyType::String);
	if (pMetaData->type != PropertyType::String) {
		// TODO: Delete the property in this case?
		pMetaData->data.iptrvalue = 0;
		return -1;
	}

	pMetaData->data.str = nstr;
	return static_cast<int>(d->map_metaData[(int)name]);
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
int RomMetaData::addMetaData_string(Property name, const string &str, unsigned int flags)
{
	if (str.empty()) {
		// Ignore empty strings.
		return -1;
	}

	string *const nstr = new string(str);
	// Trim the string if requested.
	if (nstr && (flags & STRF_TRIM_END)) {
		trimEnd(*nstr);
	}
	if (nstr->empty()) {
		// String is now empty. Ignore it.
		delete nstr;
		return -1;
	}

	RP_D(RomMetaData);
	MetaData *const pMetaData = d->addProperty(name);
	assert(pMetaData != nullptr);
	if (!pMetaData) {
		delete nstr;
		return -1;
	}

	// Make sure this is a string property.
	assert(pMetaData->type == PropertyType::String);
	if (pMetaData->type != PropertyType::String) {
		// TODO: Delete the property in this case?
		pMetaData->data.iptrvalue = 0;
		delete nstr;
		return -1;
	}

	pMetaData->data.str = nstr;
	return static_cast<int>(d->map_metaData[(int)name]);
}

/**
 * Add a timestamp metadata property.
 *
 * If a metadata property with the same name already exists,
 * it will be overwritten.
 *
 * @param name Property name
 * @param timestamp UNIX timestamp
 * @return Metadata index, or -1 on error.
 */
int RomMetaData::addMetaData_timestamp(Property name, time_t timestamp)
{
	RP_D(RomMetaData);
	MetaData *const pMetaData = d->addProperty(name);
	assert(pMetaData != nullptr);
	if (!pMetaData)
		return -1;

	// Make sure this is a timestamp property.
	assert(pMetaData->type == PropertyType::Timestamp);
	if (pMetaData->type != PropertyType::Timestamp) {
		// TODO: Delete the property in this case?
		pMetaData->data.iptrvalue = 0;
		return -1;
	}

	pMetaData->data.timestamp = timestamp;
	return static_cast<int>(d->map_metaData[(int)name]);
}

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
int RomMetaData::addMetaData_double(Property name, double dvalue)
{
	RP_D(RomMetaData);
	MetaData *const pMetaData = d->addProperty(name);
	assert(pMetaData != nullptr);
	if (!pMetaData)
		return -1;

	// Make sure this is a timestamp property.
	assert(pMetaData->type == PropertyType::Double);
	if (pMetaData->type != PropertyType::Double) {
		// TODO: Delete the property in this case?
		pMetaData->data.iptrvalue = 0;
		return -1;
	}

	pMetaData->data.dvalue = dvalue;
	return static_cast<int>(d->map_metaData[(int)name]);
}

}
