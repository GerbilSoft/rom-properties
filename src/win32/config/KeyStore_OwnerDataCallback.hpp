/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * KeyStore_OwnerDataCallback.hpp: LVS_OWNERDATA callback for Vista.       *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

// Reference: https://www.codeproject.com/Articles/35197/Undocumented-List-View-Features#virtualgroups

#ifndef __ROMPROPERTIES_WIN32_CONFIG_KEYSTORE_OWNERDATACALLBACK_HPP__
#define __ROMPROPERTIES_WIN32_CONFIG_KEYSTORE_OWNERDATACALLBACK_HPP__

#include "librpbase/common.h"
#include "libwin32common/ComBase.hpp"
#include "libwin32common/sdk/IOwnerDataCallback.hpp"

class KeyStoreWin32;
class KeyStore_OwnerDataCallback : public LibWin32Common::ComBase<IOwnerDataCallback>
{
	public:
		explicit KeyStore_OwnerDataCallback(const KeyStoreWin32 *keyStore);

	private:
		typedef LibWin32Common::ComBase<IOwnerDataCallback> super;
		RP_DISABLE_COPY(KeyStore_OwnerDataCallback)

	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) override final;

	public:
		// IOwnerDataCallback
		virtual IFACEMETHODIMP GetItemPosition(int itemIndex, LPPOINT pPosition) override final;
		virtual IFACEMETHODIMP SetItemPosition(int itemIndex, POINT position) override final;
		virtual IFACEMETHODIMP GetItemInGroup(int groupIndex, int groupWideItemIndex, PINT pTotalItemIndex) override final;
		virtual IFACEMETHODIMP GetItemGroup(int itemIndex, int occurenceIndex, PINT pGroupIndex) override final;
		virtual IFACEMETHODIMP GetItemGroupCount(int itemIndex, PINT pOccurenceCount) override final;
		virtual IFACEMETHODIMP OnCacheHint(LVITEMINDEX firstItem, LVITEMINDEX lastItem) override final;

	private:
		const KeyStoreWin32 *m_keyStore;
};

#endif /* __ROMPROPERTIES_WIN32_CONFIG_KEYSTORE_OWNERDATACALLBACK_HPP__ */
