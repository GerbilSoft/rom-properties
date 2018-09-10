/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_PropertyStore.cpp: IPropertyStore implementation.                    *
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

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_PropertyStore.hpp"
#include "RpImageWin32.hpp"

// Windows: Property keys and variants.
#include <propkey.h>
#include <propvarutil.h>

// librpbase
#include "librpbase/RomData.hpp"
#include "librpbase/RomMetaData.hpp"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/RpFile.hpp"
using namespace LibRpBase;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// libwin32common
#include "libwin32common/w32time.h"

// RpFile_IStream
#include "RpFile_IStream.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
using std::unique_ptr;
using std::wstring;

// CLSID
const CLSID CLSID_RP_PropertyStore =
	{0x4a1e3510, 0x50bd, 0x4b03, {0xa8, 0x01, 0xe4, 0xc9, 0x54, 0xf4, 0x3b, 0x96}};

/** RP_PropertyStore_Private **/
#include "RP_PropertyStore_p.hpp"

// Win32 SDK doesn't have this.
// TODO: Move to libwin32common.
// Reference: https://github.com/peirick/FlifWICCodec/blob/e42164e90ec300ae7396b6f06365ae0d7dcb651b/FlifWICCodec/decode_frame.cpp#L262
static inline HRESULT InitPropVariantFromUInt8(_In_ UCHAR uiVal, _Out_ PROPVARIANT *ppropvar)
{
	ppropvar->vt = VT_UI1;
	ppropvar->bVal = uiVal;
	return S_OK;
}
static inline HRESULT InitPropVariantFromInt8(_In_ UCHAR iVal, _Out_ PROPVARIANT *ppropvar)
{
	ppropvar->vt = VT_I1;
	ppropvar->bVal = iVal;
	return S_OK;
}

RP_PropertyStore_Private::RP_PropertyStore_Private()
	: file(nullptr)
	, pstream(nullptr)
	, grfMode(0)
{ }

RP_PropertyStore_Private::~RP_PropertyStore_Private()
{
	if (romData) {
		romData->unref();
	}

	// pstream is owned by file,
	// so don't Release() it here.
	delete file;

	// Clear property variants.
	for (auto iter = prop_val.begin(); iter != prop_val.end(); ++iter) {
		PropVariantClear(&(*iter));
	}
}

/** RP_PropertyStore **/

RP_PropertyStore::RP_PropertyStore()
	: d_ptr(new RP_PropertyStore_Private())
{ }

RP_PropertyStore::~RP_PropertyStore()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_PropertyStore::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	static const QITAB rgqit[] = {
		QITABENT(RP_PropertyStore, IInitializeWithStream),
		QITABENT(RP_PropertyStore, IPropertyStore),
		QITABENT(RP_PropertyStore, IPropertyStoreCapabilities),
		{ 0, 0 }
	};
	return LibWin32Common::pQISearch(this, rgqit, riid, ppvObj);
}

/** IInitializeWithStream **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb761812(v=vs.85).aspx [Initialize()]

IFACEMETHODIMP RP_PropertyStore::Initialize(IStream *pstream, DWORD grfMode)
{
	// Ignoring grfMode for now. (always read-only)
	RP_UNUSED(grfMode);

	// Create an IRpFile wrapper for the IStream.
	IRpFile *file = new RpFile_IStream(pstream);
	if (file->lastError() != 0) {
		// Error initializing the IRpFile.
		delete file;
		return E_FAIL;
	}

	RP_D(RP_PropertyStore);
	if (d->file) {
		// Delete the old file first.
		IRpFile *old_file = d->file;
		d->file = file;
		delete old_file;
	} else {
		// No old file to delete.
		d->file = file;
	}

	// Save the IStream and grfMode.
	d->pstream = pstream;
	d->grfMode = grfMode;

	// Attempt to create a RomData object.
	d->romData = RomDataFactory::create(file);
	if (!d->romData) {
		// No RomData.
		return E_FAIL;
	}

	// Metadata conversion table.
	// - Index: LibRpBase::Property
	// - Value:
	//   - pkey: PROPERTYKEY (if nullptr, not implemented)
	//   - vtype: Expected variant type.
	struct MetaDataConv {
		const PROPERTYKEY *pkey;
		LONG vtype;
	};
	static const MetaDataConv metaDataConv[] = {
		{nullptr, VT_EMPTY},			// Empty

		// Audio
		{&PKEY_Audio_EncodingBitrate, VT_UI4},	// BitRate (FIXME: Windows uses bit/sec; KDE uses kbit/sec)
		{&PKEY_Audio_ChannelCount, VT_UI4},	// Channels
		{&PKEY_Media_Duration, VT_UI8},		// Duration (100ns units)
		{&PKEY_Music_Genre, VT_VECTOR|VT_BSTR},	// Genre
		{&PKEY_Audio_SampleRate, VT_UI4},	// Sample rate (Hz)
		{&PKEY_Music_TrackNumber, VT_UI4},	// Track number
		{&PKEY_Media_Year, VT_UI4},		// Release year
		{&PKEY_Comment, VT_BSTR},		// Comment
		{&PKEY_Music_Artist, VT_VECTOR|VT_BSTR},// Artist
		{&PKEY_Music_AlbumTitle, VT_BSTR},	// Album
		{&PKEY_Music_AlbumArtist, VT_BSTR},	// Album artist
		{&PKEY_Music_Composer, VT_VECTOR|VT_BSTR},// Composer
		{nullptr, VT_EMPTY},			// Lyricist

		// Document
		{&PKEY_Author, VT_VECTOR|VT_BSTR},	// Author
		{&PKEY_Title, VT_BSTR},			// Title
		{&PKEY_Subject, VT_BSTR},		// Subject
		{&PKEY_SoftwareUsed, VT_BSTR},		// Generator
		{&PKEY_Document_PageCount, VT_I4},	// Page count
		{&PKEY_Document_WordCount, VT_I4},	// Word count
		{&PKEY_Document_LineCount, VT_I4},	// Line count
		{&PKEY_Language, VT_BSTR},		// Language
		{&PKEY_Copyright, VT_BSTR},		// Copyright
		{&PKEY_Company, VT_BSTR},		// Publisher (TODO: PKEY_Media_Publisher?)
		{&PKEY_DateCreated, VT_DATE},		// Creation date
		{&PKEY_Keywords, VT_VECTOR|VT_BSTR},	// Keywords

		// Media
		{&PKEY_Image_HorizontalSize, VT_UI4},	// Width
		{&PKEY_Image_VerticalSize, VT_UI4},	// Height
		{nullptr, VT_EMPTY},			// Aspect ratio (TODO)
		{&PKEY_Video_FrameRate, VT_UI4},	// Framerate (NOTE: Frames per 1000 seconds)

		// Images
		{&PKEY_Devices_Manufacturer, VT_BSTR},	// ImageMake
		{&PKEY_Devices_ModelName, VT_BSTR},	// ImageModel
		{&PKEY_Photo_DateTaken, VT_DATE},	// ImageDateTime
		{&PKEY_Photo_Orientation, VT_UI2},	// ImageOrientation
		{&PKEY_Photo_Flash, VT_UI1},		// PhotoFlash
		{nullptr, VT_EMPTY},			// PhotoPixelXDimension
		{nullptr, VT_EMPTY},			// PhotoPixelYDimension
		{nullptr, VT_EMPTY},			// PhotoDateTimeOriginal
		{nullptr, VT_EMPTY},			// PhotoFocalLength
		{nullptr, VT_EMPTY},			// PhotoFocalLengthIn35mmFilm
		{nullptr, VT_EMPTY},			// PhotoExposureTime
		{nullptr, VT_EMPTY},			// PhotoFNumber
		{nullptr, VT_EMPTY},			// PhotoApertureValue
		{nullptr, VT_EMPTY},			// PhotoExposureBiasValue
		{nullptr, VT_EMPTY},			// PhotoWhiteBalance
		{nullptr, VT_EMPTY},			// PhotoMeteringMode
		{nullptr, VT_EMPTY},			// PhotoISOSpeedRatings
		{nullptr, VT_EMPTY},			// PhotoSaturation
		{nullptr, VT_EMPTY},			// PhotoSharpness
		{nullptr, VT_EMPTY},			// PhotoGpsLatitude
		{nullptr, VT_EMPTY},			// PhotoGpsLongitude
		{nullptr, VT_EMPTY},			// PhotoGpsAltitude

		// Translations
		{nullptr, VT_EMPTY},			// TranslationUnitsTotal
		{nullptr, VT_EMPTY},			// TranslationUnitsWithTranslation
		{nullptr, VT_EMPTY},			// TranslationUnitsWithDraftTranslation
		{nullptr, VT_EMPTY},			// TranslationLastAuthor
		{nullptr, VT_EMPTY},			// TranslationLastUpDate
		{nullptr, VT_EMPTY},			// TranslationTemplateDate

		// Origin
		{nullptr, VT_EMPTY},			// OriginUrl
		{nullptr, VT_EMPTY},			// OriginEmailSubject
		{nullptr, VT_EMPTY},			// OriginEmailSender
		{nullptr, VT_EMPTY},			// OriginEmailMessageId

		// Audio
		{nullptr, VT_EMPTY},			// Disc number (FIXME: Not supported on Windows)
		{nullptr, VT_EMPTY},			// Location
		{nullptr, VT_EMPTY},			// Performer
		{nullptr, VT_EMPTY},			// Ensemble
		{nullptr, VT_EMPTY},			// Arranger
		{&PKEY_Music_Conductor, VT_VECTOR|VT_BSTR},// Conductor
		{nullptr, VT_EMPTY},			// Opus

		// Other
		{nullptr, VT_EMPTY},			// Label
		{nullptr, VT_EMPTY},			// Compilation
		{nullptr, VT_EMPTY},			// License
	};

	// Get the metadata properties.
	const RomMetaData *metaData = d->romData->metaData();
	if (!metaData || metaData->empty()) {
		// No metadata properties.
		return S_OK;
	}

	// Special handling for System.Image.Dimensions.
	SIZE dimensions = {0, 0};

	// Process the metadata.
	// TODO: Use IPropertyStoreCache?
	// Reference: https://github.com/Microsoft/Windows-classic-samples/blob/master/Samples/Win7Samples/winui/shell/appshellintegration/RecipePropertyHandler/RecipePropertyHandler.cpp
	const int count = metaData->count();
	d->prop_key.reserve(count);
	d->prop_val.reserve(count);
	for (int i = 0; i < count; i++) {
		const RomMetaData::MetaData *prop = metaData->prop(i);
		assert(prop != nullptr);
		if (!prop)
			continue;

		if (prop->name <= 0 || prop->name >= ARRAY_SIZE(metaDataConv)) {
			// FIXME: Should assert here, but Windows doesn't support
			// certain properties...
			continue;
		}

		// Convert from the RomMetaData property indexes to
		// Windows property keys.
		const MetaDataConv &conv = metaDataConv[prop->name];
		if (!conv.pkey || conv.vtype == VT_EMPTY) {
			// FIXME: Should assert here, but Windows doesn't support
			// certain properties...
			continue;
		}

		// FIXME: UIx should only accept PropertyType::UnsignedInteger,
		// and Ix should only accept PropertyType::Integer.
		PROPVARIANT prop_var;
		switch (conv.vtype) {
			case VT_UI8: {
				// FIXME: 64-bit values?
				assert(prop->type == PropertyType::Integer || prop->type == PropertyType::UnsignedInteger);
				if (prop->type != PropertyType::Integer && prop->type != PropertyType::UnsignedInteger)
					continue;

				// NOTE: Converting duration from ms to 100ns.
				if (prop->name == LibRpBase::Property::Duration) {
					uint64_t duration_100ns = static_cast<uint64_t>(prop->data.uvalue) * 10000ULL;
					InitPropVariantFromUInt64(duration_100ns, &prop_var);
				} else {
					// Use the value as-is.
					InitPropVariantFromUInt64(static_cast<uint64_t>(prop->data.uvalue), &prop_var);
				}
				d->prop_key.push_back(conv.pkey);
				d->prop_val.push_back(prop_var);
				break;
			}
			case VT_UI4:
				assert(prop->type == PropertyType::Integer || prop->type == PropertyType::UnsignedInteger);
				if (prop->type != PropertyType::Integer && prop->type != PropertyType::UnsignedInteger)
					continue;

				InitPropVariantFromUInt32(static_cast<uint32_t>(prop->data.uvalue), &prop_var);
				d->prop_key.push_back(conv.pkey);
				d->prop_val.push_back(prop_var);

				// Special handling for image dimensions.
				switch (prop->name) {
					case LibRpBase::Property::Width:
						assert(dimensions.cx == 0);
						dimensions.cx = static_cast<uint32_t>(prop->data.uvalue);
						break;
					case LibRpBase::Property::Height:
						assert(dimensions.cy == 0);
						dimensions.cy = static_cast<uint32_t>(prop->data.uvalue);
						break;
					default:
						break;
				}
				break;
			case VT_UI2:
				assert(prop->type == PropertyType::Integer || prop->type == PropertyType::UnsignedInteger);
				if (prop->type != PropertyType::Integer && prop->type != PropertyType::UnsignedInteger)
					continue;

				InitPropVariantFromUInt16(static_cast<uint16_t>(prop->data.uvalue), &prop_var);
				d->prop_key.push_back(conv.pkey);
				d->prop_val.push_back(prop_var);
				break;
			case VT_UI1:
				assert(prop->type == PropertyType::Integer || prop->type == PropertyType::UnsignedInteger);
				if (prop->type != PropertyType::Integer && prop->type != PropertyType::UnsignedInteger)
					continue;

				InitPropVariantFromUInt8(static_cast<uint8_t>(prop->data.uvalue), &prop_var);
				d->prop_key.push_back(conv.pkey);
				d->prop_val.push_back(prop_var);
				break;

			case VT_I8: {
				// FIXME: 64-bit values?
				assert(prop->type == PropertyType::Integer || prop->type == PropertyType::UnsignedInteger);
				if (prop->type != PropertyType::Integer && prop->type != PropertyType::UnsignedInteger)
					continue;

				InitPropVariantFromInt64(static_cast<int64_t>(prop->data.ivalue), &prop_var);
				d->prop_key.push_back(conv.pkey);
				d->prop_val.push_back(prop_var);
				break;
			}
			case VT_I4:
				assert(prop->type == PropertyType::Integer || prop->type == PropertyType::UnsignedInteger);
				if (prop->type != PropertyType::Integer && prop->type != PropertyType::UnsignedInteger)
					continue;

				InitPropVariantFromInt32(static_cast<int32_t>(prop->data.ivalue), &prop_var);
				d->prop_key.push_back(conv.pkey);
				d->prop_val.push_back(prop_var);
				break;
			case VT_I2:
				assert(prop->type == PropertyType::Integer || prop->type == PropertyType::UnsignedInteger);
				if (prop->type != PropertyType::Integer && prop->type != PropertyType::UnsignedInteger)
					continue;

				InitPropVariantFromInt16(static_cast<int16_t>(prop->data.ivalue), &prop_var);
				d->prop_key.push_back(conv.pkey);
				d->prop_val.push_back(prop_var);
				break;
			case VT_I1:
				assert(prop->type == PropertyType::Integer || prop->type == PropertyType::UnsignedInteger);
				if (prop->type != PropertyType::Integer && prop->type != PropertyType::UnsignedInteger)
					continue;

				InitPropVariantFromInt8(static_cast<int8_t>(prop->data.ivalue), &prop_var);
				d->prop_key.push_back(conv.pkey);
				d->prop_val.push_back(prop_var);
				break;

			case VT_BSTR:
				assert(prop->type == PropertyType::String);
				if (prop->type != PropertyType::String)
					continue;
				InitPropVariantFromString(
					prop->data.str ? U82W_s(*prop->data.str) : L"",
					&prop_var);
				d->prop_key.push_back(conv.pkey);
				d->prop_val.push_back(prop_var);
				break;

			case VT_VECTOR|VT_BSTR: {
				// For now, assuming an array with a single string.
				assert(prop->type == PropertyType::String);
				if (prop->type != PropertyType::String)
					continue;

				const wstring wstr = (prop->data.str ? U82W_s(*prop->data.str) : L"");
				const wchar_t *vstr[] = {wstr.c_str()};
				InitPropVariantFromStringVector(vstr, 1, &prop_var);
				d->prop_key.push_back(conv.pkey);
				d->prop_val.push_back(prop_var);
				break;
			}

			case VT_DATE: {
				assert(prop->type == PropertyType::Timestamp);
				if (prop->type != PropertyType::Timestamp)
					continue;

				// Date is stored as Unix time.
				// Convert to FILETIME, then to VT_DATE.
				// TODO: Verify timezone handling.
				FILETIME ft;
				UnixTimeToFileTime(prop->data.timestamp, &ft);
				InitPropVariantFromFileTime(&ft, &prop_var);
				d->prop_key.push_back(conv.pkey);
				d->prop_val.push_back(prop_var);
				break;
			}

			default:
				// TODO: Add support for multiple strings.
				// FIXME: Not supported.
				assert(!"Unsupported PROPVARIANT type.");
				break;
		}
	}

	// Special handling for System.Image.Dimensions.
	if (dimensions.cx != 0 && dimensions.cy != 0) {
		wchar_t buf[64];
		swprintf(buf, ARRAY_SIZE(buf), L"%ldx%ld", dimensions.cx, dimensions.cy);

		PROPVARIANT prop_var;
		InitPropVariantFromString(buf, &prop_var);
		d->prop_key.push_back(&PKEY_Image_Dimensions);
		d->prop_val.push_back(prop_var);
	}

	return S_OK;
}

/** IPropertyStore **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb761474(v=vs.85).aspx

IFACEMETHODIMP RP_PropertyStore::Commit(void)
{
	// All properties are read-only.
	return STG_E_ACCESSDENIED;
}

IFACEMETHODIMP RP_PropertyStore::GetAt(_In_ DWORD iProp, _Out_ PROPERTYKEY *pkey)
{
	RP_D(const RP_PropertyStore);
	if (iProp >= d->prop_key.size()) {
		return E_INVALIDARG;
	} else if (!pkey) {
		return E_POINTER;
	}

	*pkey = *d->prop_key[iProp];
	return S_OK;
}

IFACEMETHODIMP RP_PropertyStore::GetCount(_Out_ DWORD *cProps)
{
	if (!cProps) {
		return E_POINTER;
	}

	RP_D(const RP_PropertyStore);
	*cProps = static_cast<DWORD>(d->prop_key.size());
	return S_OK;
}

IFACEMETHODIMP RP_PropertyStore::GetValue(_In_ REFPROPERTYKEY key, _Out_ PROPVARIANT *pv)
{
	RP_D(const RP_PropertyStore);
	if (!pv) {
		return E_POINTER;
	}

	// Linear search for the property.
	// TODO: Optimize this?
	for (size_t n = 0; n < d->prop_key.size(); n++) {
		if (*d->prop_key[n] == key) {
			// Found the property.
			// Return the property value.
			return PropVariantCopy(pv, &d->prop_val[n]);
		}
	}

	// Property not found.
	PropVariantClear(pv);
	return S_OK;
}

IFACEMETHODIMP RP_PropertyStore::SetValue(_In_ REFPROPERTYKEY key, _In_ REFPROPVARIANT propvar)
{
	// All properties are read-only.
	RP_UNUSED(key);
	RP_UNUSED(propvar);
	return STG_E_ACCESSDENIED;
}

/** IPropertyStoreCapabilities **/
// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb761452(v=vs.85).aspx

IFACEMETHODIMP RP_PropertyStore::IsPropertyWritable(REFPROPERTYKEY key)
{
	// All properties are read-only.
	RP_UNUSED(key);
	return S_FALSE;
}
