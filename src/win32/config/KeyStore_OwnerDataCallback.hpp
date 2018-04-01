/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * KeyStore_OwnerDataCallback.hpp: LVS_OWNERDATA callback for Vista.       *
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
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) final;

	public:
		// IOwnerDataCallback
		IFACEMETHODIMP GetItemPosition(int itemIndex, LPPOINT pPosition) final;
		IFACEMETHODIMP SetItemPosition(int itemIndex, POINT position) final;
		IFACEMETHODIMP GetItemInGroup(int groupIndex, int groupWideItemIndex, PINT pTotalItemIndex) final;
		IFACEMETHODIMP GetItemGroup(int itemIndex, int occurenceIndex, PINT pGroupIndex) final;
		IFACEMETHODIMP GetItemGroupCount(int itemIndex, PINT pOccurenceCount) final;
		IFACEMETHODIMP OnCacheHint(LVITEMINDEX firstItem, LVITEMINDEX lastItem) final;

	private:
		const KeyStoreWin32 *m_keyStore;
};

#endif /* __ROMPROPERTIES_WIN32_CONFIG_KEYSTORE_OWNERDATACALLBACK_HPP__ */
