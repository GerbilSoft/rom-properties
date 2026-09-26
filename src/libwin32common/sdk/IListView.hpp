/******************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                      *
 * IListView.hpp: IListView interfaces. (undocumented)                        *
 *                                                                            *
 * Based on the Undocumented List View Features tutorial on CodeProject:      *
 * https://www.codeproject.com/Articles/35197/Undocumented-List-View-Features *
 ******************************************************************************/

#pragma once

#include "../RpWin32_sdk.h"
#include <oleidl.h>
#include <commctrl.h>

#if _WIN32_WINNT < 0x0600
# error Windows Vista SDK or later is required.
#endif

// Interface IDs
extern "C" {
	static const IID IID_IListView_WinVista =
		{0x2FFE2979, 0x5928, 0x4386, {0x9C, 0xDB, 0x8E, 0x1F, 0x15, 0xB7, 0x2F, 0xB4}};
	static const IID IID_IListView_Win7 =
		{0xE5B16AF2, 0x3990, 0x4681, {0xA6, 0x09, 0x1F, 0x06, 0x0C, 0xD1, 0x42, 0x69}};
}

// ListView message to get the IListView interface.
#define LVM_QUERYINTERFACE (LVM_FIRST + 189)
#define ListView_QueryInterface(hWnd,riid,pOut) (void)SNDMSG((hWnd),LVM_QUERYINTERFACE,(WPARAM)&(riid),(LPARAM)(pOut))

class IOwnerDataCallback;

class UUID_ATTR("{2FFE2979-5928-4386-9CDB-8E1F15B72FB4}") NOVTABLE
IListView_WinVista : public IOleWindow
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetImageList(int imageList, HIMAGELIST* pHImageList) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetImageList(int imageList, HIMAGELIST hNewImageList, HIMAGELIST* pHOldImageList) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor(COLORREF* pColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor(COLORREF color) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTextColor(COLORREF* pColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTextColor(COLORREF color) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTextBackgroundColor(COLORREF* pColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTextBackgroundColor(COLORREF color) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetHotLightColor(COLORREF* pColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetHotLightColor(COLORREF color) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetItemCount(PINT pItemCount) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetItemCount(int itemCount, DWORD flags) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetItem(LVITEMW* pItem) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetItem(LVITEMW* const pItem) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetItemState(int itemIndex, int subItemIndex, ULONG mask, ULONG* pState) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetItemState(int itemIndex, int subItemIndex, ULONG mask, ULONG state) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetItemText(int itemIndex, int subItemIndex, LPWSTR pBuffer, int bufferSize) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetItemText(int itemIndex, int subItemIndex, LPCWSTR pText) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBackgroundImage(LVBKIMAGEW* pBkImage) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetBackgroundImage(LVBKIMAGEW* const pBkImage) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFocusedColumn(PINT pColumnIndex) noexcept = 0;
	// parameters may be in wrong order
	virtual HRESULT STDMETHODCALLTYPE SetSelectionFlags(ULONG mask, ULONG flags) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSelectedColumn(PINT pColumnIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetSelectedColumn(int columnIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetView(DWORD* pView) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetView(DWORD view) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE InsertItem(LVITEMW* const pItem, PINT pItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE DeleteItem(int itemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE DeleteAllItems(void) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE UpdateItem(int itemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetItemRect(LVITEMINDEX itemIndex, int rectangleType, LPRECT pRectangle) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSubItemRect(LVITEMINDEX itemIndex, int subItemIndex, int rectangleType, LPRECT pRectangle) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE HitTestSubItem(LVHITTESTINFO* pHitTestData) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetIncrSearchString(PWSTR pBuffer, int bufferSize, PINT pCopiedChars) noexcept = 0;
	// pHorizontalSpacing and pVerticalSpacing may be in wrong order
	virtual HRESULT STDMETHODCALLTYPE GetItemSpacing(BOOL smallIconView, PINT pHorizontalSpacing, PINT pVerticalSpacing) noexcept = 0;
	// parameters may be in wrong order
	virtual HRESULT STDMETHODCALLTYPE SetIconSpacing(int horizontalSpacing, int verticalSpacing, PINT pHorizontalSpacing, PINT pVerticalSpacing) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetNextItem(LVITEMINDEX itemIndex, ULONG flags, LVITEMINDEX* pNextItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE FindItem(LVITEMINDEX startItemIndex, LVFINDINFOW const* pFindInfo, LVITEMINDEX* pFoundItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSelectionMark(LVITEMINDEX* pSelectionMark) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetSelectionMark(LVITEMINDEX newSelectionMark, LVITEMINDEX* pOldSelectionMark) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetItemPosition(LVITEMINDEX itemIndex, POINT* pPosition) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetItemPosition(int itemIndex, POINT const* pPosition) noexcept = 0;
	// parameters may be in wrong order
	virtual HRESULT STDMETHODCALLTYPE ScrollView(int horizontalScrollDistance, int verticalScrollDistance) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE EnsureItemVisible(LVITEMINDEX itemIndex, BOOL partialOk) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE EnsureSubItemVisible(LVITEMINDEX itemIndex, int subItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE EditSubItem(LVITEMINDEX itemIndex, int subItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE RedrawItems(int firstItemIndex, int lastItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE ArrangeItems(int mode) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE RecomputeItems(int unknown) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetEditControl(HWND* pHWndEdit) noexcept = 0;
	// TODO: verify that 'initialEditText' really is used to specify the initial text
	virtual HRESULT STDMETHODCALLTYPE EditLabel(LVITEMINDEX itemIndex, LPCWSTR initialEditText, HWND* phWndEdit) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE EditGroupLabel(int groupIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE CancelEditLabel(void) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetEditItem(LVITEMINDEX* itemIndex, PINT subItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE HitTest(LVHITTESTINFO* pHitTestData) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetStringWidth(PCWSTR pString, PINT pWidth) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetColumn(int columnIndex, LVCOLUMNW* pColumn) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetColumn(int columnIndex, LVCOLUMNW* const pColumn) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetColumnOrderArray(int numberOfColumns, PINT pColumns) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetColumnOrderArray(int numberOfColumns, int const* pColumns) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetHeaderControl(HWND* pHWndHeader) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE InsertColumn(int insertAt, LVCOLUMNW* const pColumn, PINT pColumnIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE DeleteColumn(int columnIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateDragImage(int itemIndex, POINT const* pUpperLeft, HIMAGELIST* pHImageList) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetViewRect(RECT* pRectangle) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetClientRect(BOOL unknown, RECT* pClientRectangle) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetColumnWidth(int columnIndex, PINT pWidth) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetColumnWidth(int columnIndex, int width) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCallbackMask(ULONG* pMask) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCallbackMask(ULONG mask) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTopIndex(PINT pTopIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCountPerPage(PINT pCountPerPage) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetOrigin(POINT* pOrigin) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSelectedCount(PINT pSelectedCount) noexcept = 0;
	// 'unknown' might specify whether to pass items' data or indexes
	virtual HRESULT STDMETHODCALLTYPE SortItems(BOOL unknown, LPARAM lParam, PFNLVCOMPARE pComparisonFunction) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetExtendedStyle(DWORD* pStyle) noexcept = 0;
	// parameters may be in wrong order
	virtual HRESULT STDMETHODCALLTYPE SetExtendedStyle(DWORD mask, DWORD style, DWORD* pOldStyle) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetHoverTime(UINT* pTime) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetHoverTime(UINT time, UINT* pOldSetting) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetToolTip(HWND* pHWndToolTip) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetToolTip(HWND hWndToolTip, HWND* pHWndOldToolTip) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetHotItem(LVITEMINDEX* pHotItem) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetHotItem(LVITEMINDEX newHotItem, LVITEMINDEX* pOldHotItem) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetHotCursor(HCURSOR* pHCursor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetHotCursor(HCURSOR hCursor, HCURSOR* pHOldCursor) noexcept = 0;
	// parameters may be in wrong order
	virtual HRESULT STDMETHODCALLTYPE ApproximateViewRect(int itemCount, PINT pWidth, PINT pHeight) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetRangeObject(int unknown, LPVOID/*ILVRange**/ pObject) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetWorkAreas(int numberOfWorkAreas, RECT* pWorkAreas) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetWorkAreas(int numberOfWorkAreas, RECT const* pWorkAreas) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetWorkAreaCount(PINT pNumberOfWorkAreas) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE ResetEmptyText(void) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE EnableGroupView(BOOL enable) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE IsGroupViewEnabled(BOOL* pIsEnabled) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SortGroups(PFNLVGROUPCOMPARE pComparisonFunction, PVOID lParam) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetGroupInfo(int unknown1, int unknown2, LVGROUP* pGroup) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetGroupInfo(int unknown, int groupID, LVGROUP* const pGroup) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetGroupRect(BOOL unknown, int groupID, int rectangleType, RECT* pRectangle) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetGroupState(int groupID, ULONG mask, ULONG* pState) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE HasGroup(int groupID, BOOL* pHasGroup) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE InsertGroup(int insertAt, LVGROUP* const pGroup, PINT pGroupID) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoveGroup(int groupID) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE InsertGroupSorted(LVINSERTGROUPSORTED const* pGroup, PINT pGroupID) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetGroupMetrics(LVGROUPMETRICS* pMetrics) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetGroupMetrics(LVGROUPMETRICS* const pMetrics) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoveAllGroups(void) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFocusedGroup(PINT pGroupID) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetGroupCount(PINT pCount) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetOwnerDataCallback(IOwnerDataCallback* pCallback) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTileViewInfo(LVTILEVIEWINFO* pInfo) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTileViewInfo(LVTILEVIEWINFO* const pInfo) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTileInfo(LVTILEINFO* pTileInfo) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTileInfo(LVTILEINFO* const pTileInfo) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetInsertMark(LVINSERTMARK* pInsertMarkDetails) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetInsertMark(LVINSERTMARK const* pInsertMarkDetails) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetInsertMarkRect(LPRECT pInsertMarkRectangle) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetInsertMarkColor(COLORREF* pColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetInsertMarkColor(COLORREF color, COLORREF* pOldColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE HitTestInsertMark(POINT const* pPoint, LVINSERTMARK* pInsertMarkDetails) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetInfoTip(LVSETINFOTIP* const pInfoTip) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetOutlineColor(COLORREF* pColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetOutlineColor(COLORREF color, COLORREF* pOldColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFrozenItem(PINT pItemIndex) noexcept = 0;
	// one parameter will be the item index; works in Icons view only
	virtual HRESULT STDMETHODCALLTYPE SetFrozenItem(int unknown1, int unknown2) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFrozenSlot(RECT* pUnknown) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetFrozenSlot(int unknown1, POINT const* pUnknown2) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetViewMargin(RECT* pMargin) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetViewMargin(RECT const* pMargin) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetKeyboardSelected(LVITEMINDEX itemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE MapIndexToId(int itemIndex, PINT pItemID) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE MapIdToIndex(int itemID, PINT pItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE IsItemVisible(LVITEMINDEX itemIndex, BOOL* pVisible) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetGroupSubsetCount(PINT pNumberOfRowsDisplayed) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetGroupSubsetCount(int numberOfRowsToDisplay) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetVisibleSlotCount(PINT pCount) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetColumnMargin(RECT* pMargin) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetSubItemCallback(LPVOID/*ISubItemCallback**/ pCallback) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetVisibleItemRange(LVITEMINDEX* pFirstItem, LVITEMINDEX* pLastItem) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTypeAheadFlags(UINT mask, UINT flags) noexcept = 0;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(IListView_WinVista, 0x2FFE2979, 0x5928, 0x4386, 0x9C, 0xDB, 0x8E, 0x1F, 0x15, 0xB7, 0x2F, 0xB4)
#endif

class UUID_ATTR("{E5B16AF2-3990-4681-A609-1F060CD14269}")
IListView_Win7 : public IOleWindow
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetImageList(int imageList, HIMAGELIST* pHImageList) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetImageList(int imageList, HIMAGELIST hNewImageList, HIMAGELIST* pHOldImageList) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor(COLORREF* pColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor(COLORREF color) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTextColor(COLORREF* pColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTextColor(COLORREF color) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTextBackgroundColor(COLORREF* pColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTextBackgroundColor(COLORREF color) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetHotLightColor(COLORREF* pColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetHotLightColor(COLORREF color) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetItemCount(PINT pItemCount) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetItemCount(int itemCount, DWORD flags) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetItem(LVITEMW* pItem) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetItem(LVITEMW* const pItem) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetItemState(int itemIndex, int subItemIndex, ULONG mask, ULONG* pState) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetItemState(int itemIndex, int subItemIndex, ULONG mask, ULONG state) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetItemText(int itemIndex, int subItemIndex, LPWSTR pBuffer, int bufferSize) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetItemText(int itemIndex, int subItemIndex, LPCWSTR pText) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBackgroundImage(LVBKIMAGEW* pBkImage) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetBackgroundImage(LVBKIMAGEW* const pBkImage) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFocusedColumn(PINT pColumnIndex) noexcept = 0;
	// parameters may be in wrong order
	virtual HRESULT STDMETHODCALLTYPE SetSelectionFlags(ULONG mask, ULONG flags) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSelectedColumn(PINT pColumnIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetSelectedColumn(int columnIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetView(DWORD* pView) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetView(DWORD view) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE InsertItem(LVITEMW* const pItem, PINT pItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE DeleteItem(int itemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE DeleteAllItems(void) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE UpdateItem(int itemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetItemRect(LVITEMINDEX itemIndex, int rectangleType, LPRECT pRectangle) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSubItemRect(LVITEMINDEX itemIndex, int subItemIndex, int rectangleType, LPRECT pRectangle) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE HitTestSubItem(LVHITTESTINFO* pHitTestData) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetIncrSearchString(PWSTR pBuffer, int bufferSize, PINT pCopiedChars) noexcept = 0;
	// pHorizontalSpacing and pVerticalSpacing may be in wrong order
	virtual HRESULT STDMETHODCALLTYPE GetItemSpacing(BOOL smallIconView, PINT pHorizontalSpacing, PINT pVerticalSpacing) noexcept = 0;
	// parameters may be in wrong order
	virtual HRESULT STDMETHODCALLTYPE SetIconSpacing(int horizontalSpacing, int verticalSpacing, PINT pHorizontalSpacing, PINT pVerticalSpacing) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetNextItem(LVITEMINDEX itemIndex, ULONG flags, LVITEMINDEX* pNextItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE FindItem(LVITEMINDEX startItemIndex, LVFINDINFOW const* pFindInfo, LVITEMINDEX* pFoundItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSelectionMark(LVITEMINDEX* pSelectionMark) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetSelectionMark(LVITEMINDEX newSelectionMark, LVITEMINDEX* pOldSelectionMark) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetItemPosition(LVITEMINDEX itemIndex, POINT* pPosition) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetItemPosition(int itemIndex, POINT const* pPosition) noexcept = 0;
	// parameters may be in wrong order
	virtual HRESULT STDMETHODCALLTYPE ScrollView(int horizontalScrollDistance, int verticalScrollDistance) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE EnsureItemVisible(LVITEMINDEX itemIndex, BOOL partialOk) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE EnsureSubItemVisible(LVITEMINDEX itemIndex, int subItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE EditSubItem(LVITEMINDEX itemIndex, int subItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE RedrawItems(int firstItemIndex, int lastItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE ArrangeItems(int mode) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE RecomputeItems(int unknown) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetEditControl(HWND* pHWndEdit) noexcept = 0;
	// TODO: verify that 'initialEditText' really is used to specify the initial text
	virtual HRESULT STDMETHODCALLTYPE EditLabel(LVITEMINDEX itemIndex, LPCWSTR initialEditText, HWND* phWndEdit) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE EditGroupLabel(int groupIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE CancelEditLabel(void) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetEditItem(LVITEMINDEX* itemIndex, PINT subItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE HitTest(LVHITTESTINFO* pHitTestData) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetStringWidth(PCWSTR pString, PINT pWidth) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetColumn(int columnIndex, LVCOLUMNW* pColumn) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetColumn(int columnIndex, LVCOLUMNW* const pColumn) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetColumnOrderArray(int numberOfColumns, PINT pColumns) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetColumnOrderArray(int numberOfColumns, int const* pColumns) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetHeaderControl(HWND* pHWndHeader) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE InsertColumn(int insertAt, LVCOLUMNW* const pColumn, PINT pColumnIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE DeleteColumn(int columnIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateDragImage(int itemIndex, POINT const* pUpperLeft, HIMAGELIST* pHImageList) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetViewRect(RECT* pRectangle) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetClientRect(BOOL unknown, RECT* pClientRectangle) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetColumnWidth(int columnIndex, PINT pWidth) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetColumnWidth(int columnIndex, int width) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCallbackMask(ULONG* pMask) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCallbackMask(ULONG mask) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTopIndex(PINT pTopIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCountPerPage(PINT pCountPerPage) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetOrigin(POINT* pOrigin) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSelectedCount(PINT pSelectedCount) noexcept = 0;
	// 'unknown' might specify whether to pass items' data or indexes
	virtual HRESULT STDMETHODCALLTYPE SortItems(BOOL unknown, LPARAM lParam, PFNLVCOMPARE pComparisonFunction) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetExtendedStyle(DWORD* pStyle) noexcept = 0;
	// parameters may be in wrong order
	virtual HRESULT STDMETHODCALLTYPE SetExtendedStyle(DWORD mask, DWORD style, DWORD* pOldStyle) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetHoverTime(UINT* pTime) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetHoverTime(UINT time, UINT* pOldSetting) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetToolTip(HWND* pHWndToolTip) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetToolTip(HWND hWndToolTip, HWND* pHWndOldToolTip) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetHotItem(LVITEMINDEX* pHotItem) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetHotItem(LVITEMINDEX newHotItem, LVITEMINDEX* pOldHotItem) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetHotCursor(HCURSOR* pHCursor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetHotCursor(HCURSOR hCursor, HCURSOR* pHOldCursor) noexcept = 0;
	// parameters may be in wrong order
	virtual HRESULT STDMETHODCALLTYPE ApproximateViewRect(int itemCount, PINT pWidth, PINT pHeight) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetRangeObject(int unknown, LPVOID/*ILVRange**/ pObject) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetWorkAreas(int numberOfWorkAreas, RECT* pWorkAreas) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetWorkAreas(int numberOfWorkAreas, RECT const* pWorkAreas) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetWorkAreaCount(PINT pNumberOfWorkAreas) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE ResetEmptyText(void) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE EnableGroupView(BOOL enable) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE IsGroupViewEnabled(BOOL* pIsEnabled) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SortGroups(PFNLVGROUPCOMPARE pComparisonFunction, PVOID lParam) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetGroupInfo(int unknown1, int unknown2, LVGROUP* pGroup) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetGroupInfo(int unknown, int groupID, LVGROUP* const pGroup) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetGroupRect(BOOL unknown, int groupID, int rectangleType, RECT* pRectangle) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetGroupState(int groupID, ULONG mask, ULONG* pState) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE HasGroup(int groupID, BOOL* pHasGroup) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE InsertGroup(int insertAt, LVGROUP* const pGroup, PINT pGroupID) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoveGroup(int groupID) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE InsertGroupSorted(LVINSERTGROUPSORTED const* pGroup, PINT pGroupID) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetGroupMetrics(LVGROUPMETRICS* pMetrics) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetGroupMetrics(LVGROUPMETRICS* const pMetrics) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoveAllGroups(void) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFocusedGroup(PINT pGroupID) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetGroupCount(PINT pCount) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetOwnerDataCallback(IOwnerDataCallback* pCallback) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTileViewInfo(LVTILEVIEWINFO* pInfo) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTileViewInfo(LVTILEVIEWINFO* const pInfo) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTileInfo(LVTILEINFO* pTileInfo) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTileInfo(LVTILEINFO* const pTileInfo) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetInsertMark(LVINSERTMARK* pInsertMarkDetails) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetInsertMark(LVINSERTMARK const* pInsertMarkDetails) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetInsertMarkRect(LPRECT pInsertMarkRectangle) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetInsertMarkColor(COLORREF* pColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetInsertMarkColor(COLORREF color, COLORREF* pOldColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE HitTestInsertMark(POINT const* pPoint, LVINSERTMARK* pInsertMarkDetails) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetInfoTip(LVSETINFOTIP* const pInfoTip) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetOutlineColor(COLORREF* pColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetOutlineColor(COLORREF color, COLORREF* pOldColor) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFrozenItem(PINT pItemIndex) noexcept = 0;
	// one parameter will be the item index; works in Icons view only
	virtual HRESULT STDMETHODCALLTYPE SetFrozenItem(int unknown1, int unknown2) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFrozenSlot(RECT* pUnknown) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetFrozenSlot(int unknown1, POINT const* pUnknown2) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetViewMargin(RECT* pMargin) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetViewMargin(RECT const* pMargin) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetKeyboardSelected(LVITEMINDEX itemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE MapIndexToId(int itemIndex, PINT pItemID) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE MapIdToIndex(int itemID, PINT pItemIndex) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE IsItemVisible(LVITEMINDEX itemIndex, BOOL* pVisible) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE EnableAlphaShadow(BOOL enable) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetGroupSubsetCount(PINT pNumberOfRowsDisplayed) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetGroupSubsetCount(int numberOfRowsToDisplay) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetVisibleSlotCount(PINT pCount) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetColumnMargin(RECT* pMargin) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetSubItemCallback(LPVOID/*ISubItemCallback**/ pCallback) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE GetVisibleItemRange(LVITEMINDEX* pFirstItem, LVITEMINDEX* pLastItem) noexcept = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTypeAheadFlags(UINT mask, UINT flags) noexcept = 0;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(IListView_Win7, 0xE5B16AF2, 0x3990, 0x4681, 0xA6, 0x09, 0x1F, 0x06, 0x0C, 0xD1, 0x42, 0x69)
#endif
