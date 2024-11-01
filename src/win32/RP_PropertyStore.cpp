/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_PropertyStore.cpp: IPropertyStore implementation.                    *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_PropertyStore.hpp"
#include "RpImageWin32.hpp"

// Windows: Property keys and variants.
#include <propkey.h>
#include <propvarutil.h>

// librpbase, librpfile, libromdata
#include "librpbase/RomMetaData.hpp"
using LibRpFile::IRpFile;
using namespace LibRpBase;
using namespace LibRomData;

// libwin32common
#include "libwin32common/propsys_xp.h"

// RpFile_IStream
#include "file/RpFile_IStream.hpp"

// C++ STL classes
using std::array;
using std::shared_ptr;
using std::wstring;

// CLSID
const CLSID CLSID_RP_PropertyStore =
	{0x4a1e3510, 0x50bd, 0x4b03, {0xa8, 0x01, 0xe4, 0xc9, 0x54, 0xf4, 0x3b, 0x96}};

/** RP_PropertyStore_Private **/
#include "RP_PropertyStore_p.hpp"

/**
 * Metadata conversion table.
 * - Index: LibRpBase::Property
 * - Value:
 *   - pkey: PROPERTYKEY (if nullptr, not implemented)
 *   - vtype: Expected variant type.
 */
const array<RP_PropertyStore_Private::MetaDataConv, 79> RP_PropertyStore_Private::metaDataConv = {{
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
	{&PKEY_Devices_Manufacturer, VT_BSTR},	// Manufacturer
	{&PKEY_Devices_ModelName, VT_BSTR},	// Model
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

	// Added in KF5 5.48
	{&PKEY_Rating, VT_UI4},			// Rating: [0,100]; convert to [1,99] for Windows.
	{&PKEY_Music_Lyrics, VT_BSTR},		// Lyrics

	// Replay gain (KF5 5.51)
	{nullptr, VT_R8},			// ReplayGainAlbumPeak
	{nullptr, VT_R8},			// ReplayGainAlbumGain
	{nullptr, VT_R8},			// ReplayGainTrackPeak
	{nullptr, VT_R8},			// ReplayGainTrackGain

	// Added in KF5 5.53
	{&PKEY_FileDescription, VT_BSTR},	// Description
}};

// Win32 SDK doesn't have this.
// TODO: Move to libwin32common.
// Reference: https://github.com/peirick/FlifWICCodec/blob/e42164e90ec300ae7396b6f06365ae0d7dcb651b/FlifWICCodec/decode_frame.cpp#L262
static inline HRESULT InitPropVariantFromUInt8(_In_ UCHAR uiVal, _Out_ PROPVARIANT *ppropvar)
{
	ppropvar->vt = VT_UI1;
	ppropvar->bVal = uiVal;
	return S_OK;
}
static inline HRESULT InitPropVariantFromInt8(_In_ CHAR iVal, _Out_ PROPVARIANT *ppropvar)
{
	ppropvar->vt = VT_I1;
	ppropvar->cVal = iVal;
	return S_OK;
}

RP_PropertyStore_Private::RP_PropertyStore_Private()
	: file(nullptr)
	, pstream(nullptr)
	, grfMode(0)
{}

RP_PropertyStore_Private::~RP_PropertyStore_Private()
{
	// Clear property variants.
	for (PROPVARIANT &pv : prop_val) {
		PropVariantClear(&pv);
	}
}

/** RP_PropertyStore **/

RP_PropertyStore::RP_PropertyStore()
	: d_ptr(new RP_PropertyStore_Private())
{}

RP_PropertyStore::~RP_PropertyStore()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/implementing-iunknown-in-c-plus-plus

IFACEMETHODIMP RP_PropertyStore::QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj)
{
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(RP_PropertyStore, IInitializeWithStream),
		QITABENT(RP_PropertyStore, IPropertyStore),
		QITABENT(RP_PropertyStore, IPropertyStoreCapabilities),
		{ 0, 0 }
	};
#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::rp_QISearch(this, rgqit, riid, ppvObj);
}

/** IInitializeWithStream **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/api/propsys/nf-propsys-iinitializewithstream-initialize [Initialize()]

IFACEMETHODIMP RP_PropertyStore::Initialize(_In_ IStream *pstream, DWORD grfMode)
{
	// Ignoring grfMode for now. (always read-only)
	RP_UNUSED(grfMode);

	// Create an IRpFile wrapper for the IStream.
	shared_ptr<RpFile_IStream> file = std::make_shared<RpFile_IStream>(pstream, true);
	if (file->lastError() != 0) {
		// Error initializing the IRpFile.
		return E_FAIL;
	}

	// Update d->file().
	// shared_ptr<> will automatically unreference the old
	// file if one is set.
	// TODO: Use shared_ptr::swap<> instead? (same for elsewhere...)
	RP_D(RP_PropertyStore);
	d->file = file;

	// Save the IStream and grfMode.
	d->pstream = pstream;
	d->grfMode = grfMode;

	// Attempt to create a RomData object.
	// TODO: Do we need to keep it open?
	d->romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_METADATA);
	if (!d->romData) {
		// No RomData.
		return E_FAIL;
	}

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
	assert(count > 0);
	d->prop_key.reserve(count);
	d->prop_val.reserve(count);
	for (const RomMetaData::MetaData &prop : *metaData) {
		if (prop.name <= Property::Invalid || static_cast<size_t>(prop.name) >= d->metaDataConv.size()) {
			// FIXME: Should assert here, but Windows doesn't support
			// certain properties...
			continue;
		}

		// Convert from the RomMetaData property indexes to
		// Windows property keys.
		const RP_PropertyStore_Private::MetaDataConv &conv = d->metaDataConv[(int)prop.name];
		if (!conv.pkey || conv.vtype == VT_EMPTY) {
			// FIXME: Should assert here, but Windows doesn't support
			// certain properties...
			continue;
		}

		// FIXME: UIx should only accept PropertyType::UnsignedInteger,
		// and Ix should only accept PropertyType::Integer.
		PROPVARIANT prop_var;
		PropVariantInit(&prop_var);
		switch (conv.vtype) {
			case VT_UI8: {
				// FIXME: 64-bit values?
				assert(prop.type == PropertyType::Integer || prop.type == PropertyType::UnsignedInteger);
				if (prop.type != PropertyType::Integer && prop.type != PropertyType::UnsignedInteger)
					continue;

				// Special handling for some properties.
				uint64_t uvalue64 = static_cast<uint64_t>(prop.data.uvalue);
				switch (prop.name) {
					case LibRpBase::Property::Duration:
						// Converting duration from ms to 100ns units.
						uvalue64 *= 10000ULL;
						break;
					default:
						break;
				}

				InitPropVariantFromUInt64(uvalue64, &prop_var);
				break;
			}

			case VT_UI4: {
				assert(prop.type == PropertyType::Integer || prop.type == PropertyType::UnsignedInteger);
				if (prop.type != PropertyType::Integer && prop.type != PropertyType::UnsignedInteger)
					continue;

				// Special handling for some properties.
				uint32_t uvalue = prop.data.uvalue;
				switch (prop.name) {
					case LibRpBase::Property::Width:
						assert(dimensions.cx == 0);
						dimensions.cx = prop.data.uvalue;
						break;
					case LibRpBase::Property::Height:
						assert(dimensions.cy == 0);
						dimensions.cy = prop.data.uvalue;
						break;
					case LibRpBase::Property::Rating:
						// Constrain to [1,99].
						if (uvalue < 1) {
							uvalue = 1;
						} else if (uvalue > 99) {
							uvalue = 99;
						}
						break;
					default:
						break;
				}

				InitPropVariantFromUInt32(uvalue, &prop_var);
				break;
			}

			case VT_UI2:
				assert(prop.type == PropertyType::Integer || prop.type == PropertyType::UnsignedInteger);
				if (prop.type != PropertyType::Integer && prop.type != PropertyType::UnsignedInteger)
					continue;

				InitPropVariantFromUInt16(static_cast<uint16_t>(prop.data.uvalue), &prop_var);
				break;

			case VT_UI1:
				assert(prop.type == PropertyType::Integer || prop.type == PropertyType::UnsignedInteger);
				if (prop.type != PropertyType::Integer && prop.type != PropertyType::UnsignedInteger)
					continue;

				InitPropVariantFromUInt8(static_cast<uint8_t>(prop.data.uvalue), &prop_var);
				break;

			case VT_I8:
				// FIXME: 64-bit values?
				assert(prop.type == PropertyType::Integer || prop.type == PropertyType::UnsignedInteger);
				if (prop.type != PropertyType::Integer && prop.type != PropertyType::UnsignedInteger)
					continue;

				InitPropVariantFromInt64(static_cast<int64_t>(prop.data.ivalue), &prop_var);
				break;

			case VT_I4:
				assert(prop.type == PropertyType::Integer || prop.type == PropertyType::UnsignedInteger);
				if (prop.type != PropertyType::Integer && prop.type != PropertyType::UnsignedInteger)
					continue;

				InitPropVariantFromInt32(static_cast<int32_t>(prop.data.ivalue), &prop_var);
				break;

			case VT_I2:
				assert(prop.type == PropertyType::Integer || prop.type == PropertyType::UnsignedInteger);
				if (prop.type != PropertyType::Integer && prop.type != PropertyType::UnsignedInteger)
					continue;

				InitPropVariantFromInt16(static_cast<int16_t>(prop.data.ivalue), &prop_var);
				break;

			case VT_I1:
				assert(prop.type == PropertyType::Integer || prop.type == PropertyType::UnsignedInteger);
				if (prop.type != PropertyType::Integer && prop.type != PropertyType::UnsignedInteger)
					continue;

				InitPropVariantFromInt8(static_cast<int8_t>(prop.data.ivalue), &prop_var);
				break;

			case VT_BSTR:
				assert(prop.type == PropertyType::String);
				if (prop.type != PropertyType::String)
					continue;
				if (prop.data.str) {
					InitPropVariantFromString(U82W_s(*prop.data.str), &prop_var);
				}
				break;

			case VT_VECTOR|VT_BSTR: {
				// For now, assuming an array with a single string.
				assert(prop.type == PropertyType::String);
				if (prop.type != PropertyType::String)
					continue;
				const wstring wstr = (prop.data.str ? U82W_s(*prop.data.str) : L"");
				const wchar_t *vstr[] = {wstr.c_str()};

				InitPropVariantFromStringVector(vstr, 1, &prop_var);
				break;
			}

			// FIXME: This is VT_FILETIME, not VT_DATE.
			// Add a proper VT_DATE handler.
			case VT_DATE: {
				assert(prop.type == PropertyType::Timestamp);
				if (prop.type != PropertyType::Timestamp)
					continue;
				// Date is stored as Unix time.
				// Convert to FILETIME, then to VT_DATE.
				// TODO: Verify timezone handling.
				FILETIME ft;
				UnixTimeToFileTime(prop.data.timestamp, &ft);

				InitPropVariantFromFileTime(&ft, &prop_var);
				break;
			}

			case VT_R8: {
				assert(prop.type == PropertyType::Double);
				if (prop.type != PropertyType::Double)
					continue;

				InitPropVariantFromDouble(prop.data.dvalue, &prop_var);
				break;
			}

			case VT_R4: {
				assert(prop.type == PropertyType::Double);
				if (prop.type != PropertyType::Double)
					continue;

				InitPropVariantFromFloat(static_cast<float>(prop.data.dvalue), &prop_var);
				break;
			}

			default:
				// TODO: Add support for multiple strings.
				// FIXME: Not supported.
				assert(!"Unsupported PROPVARIANT type.");
				break;
		}

		if (prop_var.vt != VT_EMPTY) {
			d->prop_key.emplace_back(conv.pkey);
			d->prop_val.emplace_back(std::move(prop_var));
		}
	}

	// Special handling for System.Image.Dimensions.
	if (dimensions.cx != 0 && dimensions.cy != 0) {
		wchar_t buf[64];
		swprintf(buf, _countof(buf), L"%ldx%ld", dimensions.cx, dimensions.cy);

		PROPVARIANT prop_var;
		InitPropVariantFromString(buf, &prop_var);
		d->prop_key.emplace_back(&PKEY_Image_Dimensions);
		d->prop_val.emplace_back(prop_var);
	}

	return S_OK;
}

/** IPropertyStore **/
// Reference: https://docs.microsoft.com/en-us/windows/win32/api/propsys/nn-propsys-ipropertystore

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
	PropVariantInit(pv);
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
// Reference: https://docs.microsoft.com/en-us/windows/win32/api/propsys/nn-propsys-ipropertystorecapabilities

IFACEMETHODIMP RP_PropertyStore::IsPropertyWritable(REFPROPERTYKEY key)
{
	// All properties are read-only.
	RP_UNUSED(key);
	return S_FALSE;
}
