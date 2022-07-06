/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * KeyStore_OwnerDataCallback.hpp: LVS_OWNERDATA callback for Vista.       *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://www.codeproject.com/Articles/35197/Undocumented-List-View-Features#virtualgroups

#ifndef __ROMPROPERTIES_WIN32_CONFIG_KEYSTORE_OWNERDATACALLBACK_HPP__
#define __ROMPROPERTIES_WIN32_CONFIG_KEYSTORE_OWNERDATACALLBACK_HPP__

#include "common.h"
#include "libwin32common/ComBase.hpp"
#include "libwin32common/sdk/IOwnerDataCallback.hpp"

class KeyStoreWin32;
class KeyStore_OwnerDataCallback final : public LibWin32Common::ComBase<IOwnerDataCallback>
{
	public:
		explicit KeyStore_OwnerDataCallback(const KeyStoreWin32 *keyStore);
	protected:
		virtual ~KeyStore_OwnerDataCallback() { }

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

#endif /* __ROMPROPERTIES_WIN32_CONFIG_KEYSTORE_OWNERDATACALLBACK_HPP__ */
