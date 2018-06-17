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
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/RpFile.hpp"
using namespace LibRpBase;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

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

	// TODO: Get actual properties.
	// For now, using examples.

	// Clear properties first.
	for (auto iter = d->prop_val.begin(); iter != d->prop_val.end(); ++iter) {
		PropVariantClear(&(*iter));
	}
	d->prop_key.resize(4);
	d->prop_val.resize(4);

	d->prop_key[0] = &PKEY_Title;
	InitPropVariantFromString(L"moo", &d->prop_val[0]);
	d->prop_key[1] = &PKEY_Image_Dimensions;
	InitPropVariantFromString(L"512x256", &d->prop_val[1]);
	d->prop_key[2] = &PKEY_Image_HorizontalSize;
	InitPropVariantFromUInt32(512, &d->prop_val[2]);
	d->prop_key[3] = &PKEY_Image_VerticalSize;
	InitPropVariantFromUInt32(256, &d->prop_val[3]);
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
	*cProps = d->prop_key.size();
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
	size_t n;
	for (n = 0; n < d->prop_key.size(); n++) {
		if (*d->prop_key[n] == key) {
			// Found the property.
			break;
		}
	}
	if (n >= d->prop_key.size()) {
		// Property not found.
		PropVariantClear(pv);
		return S_OK;
	}

	// Return the property value.
	return PropVariantCopy(pv, &d->prop_val[n]);
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
