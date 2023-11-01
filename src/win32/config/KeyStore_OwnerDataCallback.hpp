/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * KeyStore_OwnerDataCallback.hpp: LVS_OWNERDATA callback for Vista.       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://www.codeproject.com/Articles/35197/Undocumented-List-View-Features#virtualgroups

#pragma once

#include "common.h"
#include "libwin32common/ComBase.hpp"
#include "libwin32common/sdk/IOwnerDataCallback.hpp"

class KeyStoreWin32;
class KeyStore_OwnerDataCallback final : public LibWin32Common::ComBase<IOwnerDataCallback>
{
public:
	explicit KeyStore_OwnerDataCallback(const KeyStoreWin32 *keyStore);
protected:
	~KeyStore_OwnerDataCallback() final = default;

private:
	typedef LibWin32Common::ComBase<IOwnerDataCallback> super;
	RP_DISABLE_COPY(KeyStore_OwnerDataCallback)

public:
	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) final;

public:
	// IOwnerDataCallback
	IFACEMETHODIMP GetItemPosition(int itemIndex, LPPOINT pPosition) final;
	IFACEMETHODIMP SetItemPosition(int itemIndex, POINT position) final;
	IFACEMETHODIMP GetItemInGroup(int groupIndex, int groupWideItemIndex, PINT pTotalItemIndex) final;
	IFACEMETHODIMP GetItemGroup(int itemIndex, int occurrenceIndex, PINT pGroupIndex) final;
	IFACEMETHODIMP GetItemGroupCount(int itemIndex, PINT pOccurrenceCount) final;
	IFACEMETHODIMP OnCacheHint(LVITEMINDEX firstItem, LVITEMINDEX lastItem) final;

private:
	const KeyStoreWin32 *m_keyStore;
};
