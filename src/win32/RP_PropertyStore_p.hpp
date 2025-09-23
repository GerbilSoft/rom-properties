/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_PropertyStore_p.hpp: IPropertyStore implementation. (PRIVATE CLASS)  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "RP_PropertyStore.hpp"

// librpbase, librpfile
#include "librpbase/RomData.hpp"
#include "librpbase/RomMetaData.hpp"
#include "librpfile/IRpFile.hpp"

// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_PropertyStorePrivate RP_PropertyStore_Private

// CLSID
extern const CLSID CLSID_RP_PropertyStore;

// C++ includes
#include <array>
#include <vector>

class RP_PropertyStore_Private
{
public:
	RP_PropertyStore_Private();
	~RP_PropertyStore_Private();

private:
	RP_DISABLE_COPY(RP_PropertyStore_Private)

public:
	// Set by IInitializeWithStream::Initialize().
	LibRpFile::IRpFilePtr file;

	// RomData object
	LibRpBase::RomDataPtr romData;

	// NOTE: prop_key.pid == index + 2,
	// since pids 0 and 1 are reserved.

	// Property keys
	std::vector<const PROPERTYKEY*> prop_key;
	// Property values
	std::vector<PROPVARIANT> prop_val;

	// IStream* used by the IRpFile.
	// NOTE: Do NOT Release() this; RpFile_IStream handles it.
	IStream *pstream;
	DWORD grfMode;

	/**
	 * Metadata conversion table.
	 * - Index: LibRpBase::Property
	 * - Value:
	 *   - pkey: PROPERTYKEY (if nullptr, not implemented)
	 *   - vtype: Expected variant type.
	 */
	struct MetaDataConv {
		const PROPERTYKEY *pkey;
		LONG vtype;
	};
	static const std::array<MetaDataConv, static_cast<size_t>(LibRpBase::Property::PropertyCount)> metaDataConv;

public:
	/**
	 * Get the PreviewDetails string.
	 * @return PreviewDetails string.
	 */
	static std::tstring GetPreviewDetailsString(void);

	/**
	 * Get the InfoTip string.
	 * @return InfoTip string.
	 */
	static std::tstring GetInfoTipString(void);

	/**
	 * Get the FullDetails string.
	 * @return FullDetails string.
	 */
	static std::tstring GetFullDetailsString(void);

public:
	/**
	 * Get the Property Description Schema directory.
	 * @return Property Description Schema directory
	 */
	static std::tstring GetPropertyDescriptionSchemaDirectory(void);
};
