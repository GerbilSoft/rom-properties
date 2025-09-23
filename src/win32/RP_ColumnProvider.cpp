/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ColumnProvider.cpp: IColumnProvider implementation.                  *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_ColumnProvider.hpp"
#include "RpImageWin32.hpp"

// Custom properties
// NOTE: Using the same GUIDs for IColumnProvider as IPropertyStore.
#include "RP_PropertyStore_GUIDs.h"

// librpbase, librpfile, libromdata
#include "librpbase/config/Config.hpp"
#include "librpbase/RomMetaData.hpp"
#include "librpfile/FileSystem.hpp"
#include "libromdata/RomDataFactory.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRomData;

// C++ STL classes
using std::array;
using std::shared_ptr;
using std::wstring;

// CLSID
const CLSID CLSID_RP_ColumnProvider =
	{0x126621f9, 0x01e7, 0x45da, {0xbc, 0x4f, 0xcb, 0xdf, 0xab, 0x9c, 0x0e, 0x0a}};

/** RP_ColumnProvider_Private **/
#include "RP_ColumnProvider_p.hpp"

// We're handling all custom metadata properties as columns.
// NOTE: Using custom structs because we have to use a pointer
// for the GUIDs.

struct SHCOLUMNINFO_NoSCID {
	VARTYPE vt;
	DWORD fmt;
	UINT cChars;
	DWORD csFlags;
	LPCWSTR lpwszTitle;
	LPCWSTR lpwszDescription;
};

// TODO: Localize.
static const array<SHCOLUMNINFO_NoSCID, 6> coldata_t = {{
	{VT_BSTR, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR, _T("Game ID"), _T("Game ID")},
	{VT_BSTR, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR, _T("Title ID"), _T("Title ID")},
	{VT_BSTR, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR, _T("Media ID"), _T("Media ID")},
	{VT_BSTR, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR, _T("OS Version"), _T("OS Version")},
	{VT_BSTR, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR, _T("Encryption Key"), _T("Encryption Key")},
	{VT_BSTR, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR, _T("Pixel Format"), _T("Pixel Format")},
}};

// Property keys
static const array<const PROPERTYKEY*, 6> colpkey_t = {{
	&PKEY_RomProperties_GameID,
	&PKEY_RomProperties_TitleID,
	&PKEY_RomProperties_MediaID,
	&PKEY_RomProperties_OSVersion,
	&PKEY_RomProperties_EncryptionKey,
	&PKEY_RomProperties_PixelFormat,
}};

/** RP_ColumnProvider **/

RP_ColumnProvider::RP_ColumnProvider()
	: d_ptr(new RP_ColumnProvider_Private())
{}

RP_ColumnProvider::~RP_ColumnProvider()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/implementing-iunknown-in-c-plus-plus

IFACEMETHODIMP RP_ColumnProvider::QueryInterface(_In_ REFIID riid, _Outptr_ LPVOID *ppvObj)
{
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(RP_ColumnProvider, IColumnProvider),
		{ 0, 0 }
	};
#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::rp_QISearch(this, rgqit, riid, ppvObj);
}

/** IColumnProvider **/
// Reference: https://learn.microsoft.com/en-us/windows/win32/api/shlobj/nn-shlobj-icolumnprovider

IFACEMETHODIMP RP_ColumnProvider::Initialize(LPCSHCOLUMNINIT psci)
{
	// Check if the directory is on a "bad" file system.
	Config *const config = Config::instance();
	if (FileSystem::isOnBadFS(psci->wszFolder, config->getBoolConfigOption(Config::BoolConfig::Options_EnableThumbnailOnNetworkFS))) {
		// This directory is on a "bad" file system.
		// TODO: Better HRESULT?
		return E_FAIL;
	}

	return S_OK;
}

IFACEMETHODIMP RP_ColumnProvider::GetColumnInfo(_In_ DWORD dwIndex, _Out_ SHCOLUMNINFO *psci)
{
	static_assert(coldata_t.size() == colpkey_t.size(), "coldata_t.size() != colpkey_t.size()");

	if (dwIndex >= coldata_t.size()) {
		// Out of range.
		return S_FALSE;
	}

	const SHCOLUMNINFO_NoSCID &sci = coldata_t[dwIndex];

	psci->scid = *colpkey_t[dwIndex];
	psci->vt = sci.vt;
	psci->fmt = sci.fmt;
	psci->cChars = sci.cChars;
	psci->csFlags = sci.csFlags;
	wcscpy(psci->wszTitle, sci.lpwszTitle);
	wcscpy(psci->wszDescription, sci.lpwszDescription);

	return S_OK;
}

IFACEMETHODIMP RP_ColumnProvider::GetItemData(_In_ LPCSHCOLUMNID pscid, _In_ LPCSHCOLUMNDATA pscd, _Out_ VARIANT *pvarData)
{
	// Map the specified property key to a Property enum value.
	Property name = Property::Invalid;
	for (size_t i = 0; i < colpkey_t.size(); i++) {
		if (!memcmp(pscid, colpkey_t[i], sizeof(SHCOLUMNID))) {
			// Found a match!
			name = static_cast<Property>(static_cast<int>(Property::GameID) + static_cast<int>(i));
			break;
		}
	}

	if (name <= Property::Empty) {
		// Invalid property.
		return S_FALSE;
	}

	// Check if we have a cached RomData object and if the filename matches.
	RP_D(RP_ColumnProvider);
	if (d->tfilename != pscd->wszFile) {
		// Cache doesn't match. Open the new file.
		// NOTE: If d->romData is nullptr after this, this will be considered a "negative" cache.
		d->romData = RomDataFactory::create(pscd->wszFile, RomDataFactory::RDA_HAS_METADATA);
		d->tfilename = pscd->wszFile;
	}

	if (!d->romData) {
		// Not a supported RomData object.
		return S_FALSE;
	}

	// Get the custom metadata properties.
	const RomMetaData *const metaData = d->romData->metaData();
	if (!metaData || metaData->empty()) {
		// No metadata properties.
		return S_FALSE;
	}
	// The file doesn't need to stay open after retrieving the metadata properties.
	d->romData->close();

	const RomMetaData::MetaData *const prop = metaData->get(name);
	if (!prop) {
		// Property not found.
		return S_FALSE;
	}

	// NOTE: Assuming it's a string.
	assert(prop->type == PropertyType::String);
	if (prop->type != PropertyType::String) {
		// Not supported right now...
		return S_FALSE;
	}

	if (!prop->data.str || prop->data.str[0] == '\0') {
		// Empty string...
		// NOTE: Shouldn't happen.
		return S_FALSE;
	}

	InitVariantFromString(U82T_c(prop->data.str), pvarData);
	return S_OK;
}
