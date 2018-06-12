/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomMetaData.cpp: ROM metadata class.                                    *
 *                                                                         *
 * Unlike RomMetaData, which shows all of the information of a ROM image   *
 * in a generic list, RomMetaData stores specific properties that can be   *
 * used by the desktop environment's indexer.                              *
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
	private:
		~RomMetaDataPrivate();	// call unref() instead

	private:
		RP_DISABLE_COPY(RomMetaDataPrivate)

	public:
		/** Reference count functions. **/

		/**
		 * Create a reference of this object.
		 * @return this
		 */
		RomMetaDataPrivate *ref(void);

		/**
		 * Unreference this object.
		 */
		void unref(void);

		/**
		 * Is this object currently shared?
		 * @return True if ref_cnt > 1; false if not.
		 */
		inline bool isShared(void) const;

	private:
		// Current reference count.
		volatile int ref_cnt;

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
	PropertyType::Integer,	// Track number
	PropertyType::Integer,	// Release Year
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
	PropertyType::Invalid,	// CreationDate (FIXME)
	PropertyType::Invalid,	// Keywords (FIXME)

	// Media
	PropertyType::Integer,	// Width
	PropertyType::Integer,	// Height
	PropertyType::Invalid,	// AspectRatio (FIXME: Float?)
	PropertyType::Integer,	// FrameRate

	// Images
	PropertyType::String,	// ImageMake
	PropertyType::String,	// ImageModel
	PropertyType::Invalid,	// ImageDateTime (FIXME)
	PropertyType::Invalid,	// ImageOrientation (FIXME)
	PropertyType::Invalid,	// PhotoFlash (FIXME)
	PropertyType::Invalid,	// PhotoPixelXDimension (FIXME)
	PropertyType::Invalid,	// PhotoPixelYDimension (FIXME)
	PropertyType::Invalid,	// PhotoDateTimeOriginal (FIXME)
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
	PropertyType::Integer,	// DiscNumber
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
	: ref_cnt(1)
{
	static_assert(ARRAY_SIZE(RomMetaDataPrivate::PropertyTypeMap) == Property::PropertyCount,
		      "PropertyTypeMap needs to be updated!");
}

RomMetaDataPrivate::~RomMetaDataPrivate()
{
	delete_data();
}

/**
 * Create a reference of this object.
 * @return this
 */
RomMetaDataPrivate *RomMetaDataPrivate::ref(void)
{
	ATOMIC_INC_FETCH(&ref_cnt);
	return this;
}

/**
 * Unreference this object.
 */
void RomMetaDataPrivate::unref(void)
{
	assert(ref_cnt > 0);
	if (ATOMIC_DEC_FETCH(&ref_cnt) <= 0) {
		// All references removed.
		delete this;
	}
}

/**
 * Is this object currently shared?
 * @return True if ref_cnt > 1; false if not.
 */
inline bool RomMetaDataPrivate::isShared(void) const
{
	assert(ref_cnt > 0);
	return (ref_cnt > 1);
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
	d_ptr->unref();
}

/**
 * Copy constructor.
 * @param other Other instance.
 */
RomMetaData::RomMetaData(const RomMetaData &other)
	: d_ptr(other.d_ptr->ref())
{ }

/**
 * Assignment operator.
 * @param other Other instance.
 * @return This instance.
 */
RomMetaData &RomMetaData::operator=(const RomMetaData &other)
{
	RomMetaDataPrivate *const d_old = this->d_ptr;
	this->d_ptr = other.d_ptr->ref();
	d_old->unref();
	return *this;
}

/**
 * Detach this instance from all other instances.
 * TODO: Move to RomMetaDataPrivate?
 */
void RomMetaData::detach(void)
{
	if (!d_ptr->isShared()) {
		// Only one reference.
		// Nothing to detach from.
		return;
	}

	// Need to detach.
	RomMetaDataPrivate *const d_new = new RomMetaDataPrivate();
	RomMetaDataPrivate *const d_old = d_ptr;
	d_new->metaData.resize(d_old->metaData.size());
	for (int i = static_cast<int>(d_old->metaData.size() - 1); i >= 0; i--) {
		const MetaData &metaData_old = d_old->metaData.at(i);
		MetaData &metaData_new = d_new->metaData.at(i);
		metaData_new.name = metaData_old.name;

		assert(metaData_new.name > Property::FirstProperty);
		assert(metaData_new.name < Property::PropertyCount);
		if (metaData_new.name <= Property::FirstProperty ||
		    metaData_new.name >= Property::PropertyCount)
		{
			continue;
		}

		metaData_new.type = metaData_old.type;
		switch (metaData_old.type) {
			case PropertyType::Integer:
				metaData_new.data.value = metaData_old.data.value;
				break;

			case PropertyType::String:
				metaData_new.data.str = new string(*metaData_old.data.str);
				break;

			default:
				// ERROR!
				assert(!"Unsupported RomMetaData PropertyType.");
				break;
		}
	}

	// Detached.
	d_ptr = d_new;
	d_old->unref();
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

		int idx = static_cast<int>(d->metaData.size());
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
				metaData.data.value = src->data.value;
				break;

			case PropertyType::String:
				metaData.data.str = new string(*src->data.str);
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
	int idx = static_cast<int>(d->metaData.size());
	d->metaData.resize(idx+1);
	MetaData &metaData = d->metaData.at(idx);

	metaData.name = name;
	metaData.type = PropertyType::Integer;
	metaData.data.value = value;
	return idx;
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
	int idx = static_cast<int>(d->metaData.size());
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
	return idx;
}

/**
 * Add a string metadata property.
 * @param name Metadata name.
 * @param str String value.
 * @param flags Formatting flags.
 * @return Metadata index.
 */
int RomMetaData::addMetaData_string(Property::Property name, const std::string &str, unsigned int flags)
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
	int idx = static_cast<int>(d->metaData.size());
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
	return idx;
}

}
