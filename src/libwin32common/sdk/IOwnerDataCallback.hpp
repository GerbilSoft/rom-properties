/******************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                      *
 * IOwnerDataCallback.hpp: IOwnerDataCallback interface. (undocumented)       *
 *                                                                            *
 * Based on the Undocumented List View Features tutorial on CodeProject:      *
 * https://www.codeproject.com/Articles/35197/Undocumented-List-View-Features *
 ******************************************************************************/

#pragma once

// IListView.hpp is required for LVITEMINDEX if
// _WIN32_WINNT < 0x0600.
#include "IListView.hpp"

#include "../RpWin32_sdk.h"
#include <unknwn.h>

// Interface IDs
extern "C" {
	static const IID IID_IOwnerDataCallback =
		{0x44C09D56, 0x8D3B, 0x419D, {0xA4, 0x62, 0x7B, 0x95, 0x6B, 0x10, 0x5B, 0x47}};
}

class UUID_ATTR("{44C09D56-8D3B-419D-A462-7B956B105B47}") NOVTABLE
IOwnerDataCallback : public IUnknown
{
public:
	/// \brief <em>TODO</em>
	///
	/// TODO
	///
	/// \return An \c HRESULT error code.
	virtual HRESULT STDMETHODCALLTYPE GetItemPosition(int itemIndex, LPPOINT pPosition) = 0;
	/// \brief <em>TODO</em>
	///
	/// TODO
	///
	/// \return An \c HRESULT error code.
	virtual HRESULT STDMETHODCALLTYPE SetItemPosition(int itemIndex, POINT position) = 0;
	/// \brief <em>Will be called to retrieve an item's zero-based control-wide index</em>
	///
	/// This method is called by the listview control to retrieve an item's zero-based control-wide index.
	/// The item is identified by a zero-based group index, which identifies the listview group in which
	/// the item is displayed, and a zero-based group-wide item index, which identifies the item within its
	/// group.
	///
	/// \param[in] groupIndex The zero-based index of the listview group containing the item.
	/// \param[in] groupWideItemIndex The item's zero-based group-wide index within the listview group
	///            specified by \c groupIndex.
	/// \param[out] pTotalItemIndex Receives the item's zero-based control-wide index.
	///
	/// \return An \c HRESULT error code.
	virtual HRESULT STDMETHODCALLTYPE GetItemInGroup(int groupIndex, int groupWideItemIndex, PINT pTotalItemIndex) = 0;
	/// \brief <em>Will be called to retrieve the group containing a specific occurrence of an item</em>
	///
	/// This method is called by the listview control to retrieve the listview group in which the specified
	/// occurrence of the specified item is displayed.
	///
	/// \param[in] itemIndex The item's zero-based (control-wide) index.
	/// \param[in] occurrenceIndex The zero-based index of the item's copy for which the group membership is
	///            retrieved.
	/// \param[out] pGroupIndex Receives the zero-based index of the listview group that shall contain the
	///             specified copy of the specified item.
	///
	/// \return An \c HRESULT error code.
	virtual HRESULT STDMETHODCALLTYPE GetItemGroup(int itemIndex, int occurrenceIndex, PINT pGroupIndex) = 0;
	/// \brief <em>Will be called to determine how often an item occurs in the listview control</em>
	///
	/// This method is called by the listview control to determine how often the specified item occurs in the
	/// listview control.
	///
	/// \param[in] itemIndex The item's zero-based (control-wide) index.
	/// \param[out] pOccurrencesCount Receives the number of occurrences of the item in the listview control.
	///
	/// \return An \c HRESULT error code.
	virtual HRESULT STDMETHODCALLTYPE GetItemGroupCount(int itemIndex, PINT pOccurrenceCount) = 0;
	/// \brief <em>Will be called to prepare the client app that the data for a certain range of items will be required very soon</em>
	///
	/// This method is similar to the \c LVN_ODCACHEHINT notification. It tells the client application that
	/// it should preload the details for a certain range of items because the listview control is about to
	/// request these details. The difference to \c LVN_ODCACHEHINT is that this method identifies the items
	/// by their zero-based group-wide index and the zero-based index of the listview group containing the
	/// item.
	///
	/// \param[in] firstItem The first item to cache.
	/// \param[in] lastItem The last item to cache.
	///
	/// \return An \c HRESULT error code.
	virtual HRESULT STDMETHODCALLTYPE OnCacheHint(LVITEMINDEX firstItem, LVITEMINDEX lastItem) = 0;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(IOwnerDataCallback, 0x44C09D56, 0x8D3B, 0x419D, 0xA4, 0x62, 0x7B, 0x95, 0x6B, 0x10, 0x5B, 0x47)
#endif
