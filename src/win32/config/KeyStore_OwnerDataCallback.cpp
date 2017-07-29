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
#include "stdafx.h"
#include "KeyStore_OwnerDataCallback.hpp"
#include "KeyStoreWin32.hpp"

KeyStore_OwnerDataCallback::KeyStore_OwnerDataCallback(const KeyStoreWin32 *keyStore)
	: m_keyStore(keyStore)
{ }

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP KeyStore_OwnerDataCallback::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	static const QITAB rgqit[] = {
		QITABENT(KeyStore_OwnerDataCallback, IOwnerDataCallback),
		{ 0 }
	};
	return LibWin32Common::pQISearch(this, rgqit, riid, ppvObj);
}

/** KeyStore_OwnerDataCallback **/
// Reference: https://www.codeproject.com/Articles/35197/Undocumented-List-View-Features#virtualgroups

IFACEMETHODIMP KeyStore_OwnerDataCallback::GetItemPosition(int itemIndex, LPPOINT pPosition)
{
	RP_UNUSED(itemIndex);
	RP_UNUSED(pPosition);
	return E_NOTIMPL;
}

IFACEMETHODIMP KeyStore_OwnerDataCallback::SetItemPosition(int itemIndex, POINT position)
{
	RP_UNUSED(itemIndex);
	RP_UNUSED(position);
	return E_NOTIMPL;
}

/**
 * Get the flat key index of the specified section and key index.
 * @param groupIndex		[in] Section index.
 * @param groupWideItemIndex	[in] Key index.
 * @param pTotalItemIndex	[out] Flat key index.
 */
IFACEMETHODIMP KeyStore_OwnerDataCallback::GetItemInGroup(int groupIndex, int groupWideItemIndex, PINT pTotalItemIndex)
{
	if (!pTotalItemIndex)
		return E_POINTER;
	else if (!m_keyStore)
		return E_UNEXPECTED;
	*pTotalItemIndex = m_keyStore->sectKeyToIdx(groupIndex, groupWideItemIndex);
	return S_OK;
}

/**
 * Get the section index of the specified flat key index.
 * @param itemIndex		[in] Flat key index.
 * @param occurrenceIndex	[in] Instance of the item. (usually 0 here)
 * @param pGroupIndex		[out] Section index.
 */
IFACEMETHODIMP KeyStore_OwnerDataCallback::GetItemGroup(int itemIndex, int occurenceIndex, PINT pGroupIndex)
{
	// TODO: Handle this.
	if (!pGroupIndex)
		return E_POINTER;
	else if (!m_keyStore)
		return E_UNEXPECTED;
	int sectIdx, keyIdx;
	int ret = m_keyStore->idxToSectKey(itemIndex, &sectIdx, &keyIdx);
	if (ret != 0)
		return E_INVALIDARG;
	*pGroupIndex = sectIdx;
	return S_OK;
}

IFACEMETHODIMP KeyStore_OwnerDataCallback::GetItemGroupCount(int itemIndex, PINT pOccurenceCount)
{
	// Items only appear in a single group.
	RP_UNUSED(itemIndex);
	if (!pOccurenceCount)
		return E_POINTER;
	*pOccurenceCount = 1;
	return S_OK;
}

IFACEMETHODIMP KeyStore_OwnerDataCallback::OnCacheHint(LVITEMINDEX firstItem, LVITEMINDEX lastItem)
{
	RP_UNUSED(firstItem);
	RP_UNUSED(lastItem);
	return E_NOTIMPL;
}
