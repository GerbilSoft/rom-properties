/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * commctrl_ts.h: Typesafe inline function wrappers for commctrl.h.        *
 *                                                                         *
 * Based on commctrl.h from the Windows SDK v7.1A.                         *
 ***************************************************************************/

#pragma once

#include "tsbase.h"
#include <commctrl.h>

//====== IMAGE APIS ===========================================================
#ifndef NOIMAGEAPIS

#undef ImageList_AddIcon
static FORCEINLINE int ImageList_AddIcon(_In_ HIMAGELIST himl, _In_ HICON hicon)
{
	return ImageList_ReplaceIcon(himl, -1, hicon);
}

#undef ImageList_RemoveAll
static FORCEINLINE BOOL ImageList_RemoveAll(_In_ HIMAGELIST himl)
{
	return ImageList_Remove(himl, -1);
}

#undef ImageList_ExtractIcon
static FORCEINLINE HICON ImageList_ExtractIcon(_In_ HINSTANCE hi, _In_ HIMAGELIST himl, _In_ int i)
{
	((void)hi);
	return ImageList_GetIcon(himl, i, 0);
}

#undef ImageList_LoadBitmap
static FORCEINLINE HIMAGELIST ImageList_LoadBitmapA(_In_ HINSTANCE hi, _In_ LPCSTR lpbmp, _In_ int cx, _In_ int cGrow, _In_ COLORREF crMask)
{
	return ImageList_LoadImageA(hi, lpbmp, cx, cGrow, crMask, IMAGE_BITMAP, 0);
}
static FORCEINLINE HIMAGELIST ImageList_LoadBitmapW(_In_ HINSTANCE hi, _In_ LPCWSTR lpbmp, _In_ int cx, _In_ int cGrow, _In_ COLORREF crMask)
{
	return ImageList_LoadImageW(hi, lpbmp, cx, cGrow, crMask, IMAGE_BITMAP, 0);
}

#ifdef UNICODE
# define ImageList_LoadBitmap ImageList_LoadBitmapW
#else
# define ImageList_LoadBitmap ImageList_LoadBitmapA
#endif

#endif /* NOIMAGEAPIS */

//====== HEADER CONTROL =======================================================
#ifndef NOHEADER

#undef Header_GetItemCount
static FORCEINLINE int Header_GetItemCount(_In_ HWND hwndHD)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_GETITEMCOUNT, 0, 0L))); }

#undef Header_InsertItem
static FORCEINLINE int Header_InsertItem(_In_ HWND hwndHD, _In_ int index, _In_ const LPHDITEM phdi)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_INSERTITEM, STATIC_CAST(WPARAM)(index), REINTERPRET_CAST(LPARAM)(phdi)))); }

#undef Header_DeleteItem
static FORCEINLINE BOOL Header_DeleteItem(_In_ HWND hwndHD, _In_ int index)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_DELETEITEM, STATIC_CAST(WPARAM)(index), 0L))); }

#undef Header_GetItem
static FORCEINLINE BOOL Header_GetItem(_In_ HWND hwndHD, _In_ int index, _Out_ LPHDITEM phdi)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_GETITEM, STATIC_CAST(WPARAM)(index), REINTERPRET_CAST(LPARAM)(phdi)))); }

#undef Header_SetItem
static FORCEINLINE BOOL Header_SetItem(_In_ HWND hwndHD, _In_ int index, _In_ const LPHDITEM phdi)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_SETITEM, STATIC_CAST(WPARAM)(index), REINTERPRET_CAST(LPARAM)(phdi)))); }

#undef Header_Layout
static FORCEINLINE BOOL Header_Layout(_In_ HWND hwndHD, _Out_ LPHDLAYOUT pLayout)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_LAYOUT, 0, REINTERPRET_CAST(LPARAM)(pLayout)))); }

#if (_WIN32_IE >= 0x0300)

#undef Header_GetItemRect
static FORCEINLINE BOOL Header_GetItemRect(_In_ HWND hwndHD, _In_ int iItem, _Out_ LPRECT lprc)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_GETITEMRECT, STATIC_CAST(WPARAM)(iItem), REINTERPRET_CAST(LPARAM)(lprc)))); }

#undef Header_SetImageList
static FORCEINLINE HIMAGELIST Header_SetImageList(_In_ HWND hwndHD, HIMAGELIST himl)
	{ return REINTERPRET_CAST(HIMAGELIST)(SNDMSG(hwndHD, HDM_SETIMAGELIST, HDSIL_NORMAL, REINTERPRET_CAST(LPARAM)(himl))); }
#undef Header_SetStateImageList
static FORCEINLINE HIMAGELIST Header_SetStateImageList(_In_ HWND hwndHD, HIMAGELIST himl)
	{ return REINTERPRET_CAST(HIMAGELIST)(SNDMSG(hwndHD, HDM_SETIMAGELIST, HDSIL_STATE, REINTERPRET_CAST(LPARAM)(himl))); }

#undef Header_GetImageList
static FORCEINLINE HIMAGELIST Header_GetImageList(_In_ HWND hwndHD)
	{ return REINTERPRET_CAST(HIMAGELIST)(SNDMSG(hwndHD, HDM_SETIMAGELIST, HDSIL_NORMAL, 0)); }
#undef Header_GetStateImageList
static FORCEINLINE HIMAGELIST Header_GetStateImageList(_In_ HWND hwndHD)
	{ return REINTERPRET_CAST(HIMAGELIST)(SNDMSG(hwndHD, HDM_SETIMAGELIST, HDSIL_STATE, 0)); }

#undef Header_OrderToIndex
static FORCEINLINE int Header_OrderToIndex(_In_ HWND hwndHD, _In_ int iOrder)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_ORDERTOINDEX, STATIC_CAST(WPARAM)(iOrder), 0))); }

#undef Header_CreateDragImage
static FORCEINLINE HIMAGELIST Header_CreateDragImage(_In_ HWND hwndHD, _In_ int iIndex)
	{ return REINTERPRET_CAST(HIMAGELIST)(SNDMSG(hwndHD, HDM_CREATEDRAGIMAGE, STATIC_CAST(WPARAM)(iIndex), 0)); }

#undef Header_GetOrderArray
static FORCEINLINE BOOL Header_GetOrderArray(_In_ HWND hwndHD, _In_ int iCount, _In_ int *lpiArray)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_GETORDERARRAY, STATIC_CAST(WPARAM)(iCount), REINTERPRET_CAST(LPARAM)(lpiArray)))); }

#undef Header_SetOrderArray
static FORCEINLINE BOOL Header_SetOrderArray(_In_ HWND hwndHD, _In_ int iCount, _In_ const int *lpiArray)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_SETORDERARRAY, STATIC_CAST(WPARAM)(iCount), REINTERPRET_CAST(LPARAM)(lpiArray)))); }

#undef Header_SetHotDivider
static FORCEINLINE int Header_SetHotDivider(_In_ HWND hwndHD, _In_ BOOL fPos, _In_ DWORD dwInputValue)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_SETHOTDIVIDER, STATIC_CAST(WPARAM)(fPos), STATIC_CAST(LPARAM)(dwInputValue)))); }
#endif      // _WIN32_IE >= 0x0300

#if (_WIN32_IE >= 0x0500)
#undef Header_SetBitmapMargin
static FORCEINLINE int Header_SetBitmapMargin(_In_ HWND hwndHD, _In_ int iWidth)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_SETBITMAPMARGIN, STATIC_CAST(WPARAM)(iWidth), 0))); }

#undef Header_GetBitmapMargin
static FORCEINLINE int Header_GetBitmapMargin(_In_ HWND hwndHD)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_GETBITMAPMARGIN, 0, 0))); }
#endif


#if (_WIN32_IE >= 0x0400)
#undef Header_SetUnicodeFormat
static FORCEINLINE BOOL Header_SetUnicodeFormat(_In_ HWND hwndHD, _In_ BOOL fUnicode)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_SETUNICODEFORMAT, STATIC_CAST(WPARAM)(fUnicode), 0))); }

#undef Header_GetUnicodeFormat
static FORCEINLINE BOOL Header_GetUnicodeFormat(_In_ HWND hwndHD)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_GETUNICODEFORMAT, 0, 0))); }
#endif

#if (_WIN32_IE >= 0x0500)
#undef Header_SetFilterChangeTimeout
static FORCEINLINE int Header_SetFilterChangeTimeout(_In_ HWND hwndHD, _In_ int i)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_SETFILTERCHANGETIMEOUT, 0, STATIC_CAST(LPARAM)(i)))); }

#undef Header_EditFilter
static FORCEINLINE int Header_EditFilter(_In_ HWND hwndHD, _In_ int i, _In_ BOOL fDiscardChanges)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_EDITFILTER, STATIC_CAST(WPARAM)(i), MAKELPARAM(fDiscardChanges, 0)))); }

#undef Header_ClearFilter
static FORCEINLINE int Header_ClearFilter(_In_ HWND hwndHD, _In_ int i)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_CLEARFILTER, STATIC_CAST(WPARAM)(i), 0))); }
#undef Header_ClearAllFilters
static FORCEINLINE int Header_ClearAllFilters(_In_ HWND hwndHD)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_CLEARFILTER, STATIC_CAST(WPARAM)(-1), 0))); }
#endif

#if (_WIN32_WINNT >= 0x600)
#undef Header_GetItemDropDownRect
static FORCEINLINE BOOL Header_GetItemDropDownRect(_In_ HWND hwndHD, _In_ int iItem, _Inout_ LPRECT lpItemRect)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_GETITEMDROPDOWNRECT, STATIC_CAST(WPARAM)(iItem), REINTERPRET_CAST(LPARAM)(lpItemRect)))); }

#undef Header_GetOverflowRect
static FORCEINLINE BOOL Header_GetOverflowRect(_In_ HWND hwndHD, _Inout_ LPRECT lpItemRect)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_GETOVERFLOWRECT, 0, REINTERPRET_CAST(LPARAM)(lpItemRect)))); }

#undef Header_GetFocusedItem
static FORCEINLINE int Header_GetFocusedItem(_In_ HWND hwndHD)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_GETFOCUSEDITEM, 0, 0))); }

#undef Header_SetFocusedItem
static FORCEINLINE BOOL Header_SetFocusedItem(_In_ HWND hwndHD, _In_ int iItem)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndHD, HDM_SETFOCUSEDITEM, 0, STATIC_CAST(LPARAM)(iItem)))); }
#endif // _WIN32_WINNT >= 0x600

#endif /* NOHEADER */

//====== LISTVIEW CONTROL =====================================================
#ifndef NOLISTVIEW

#if (_WIN32_IE >= 0x0400)
#undef ListView_SetUnicodeFormat
static FORCEINLINE BOOL ListView_SetUnicodeFormat(_In_ HWND hwnd, _In_ BOOL fUnicode)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_SETUNICODEFORMAT, STATIC_CAST(WPARAM)(fUnicode), 0))); }

#undef ListView_GetUnicodeFormat
static FORCEINLINE BOOL ListView_GetUnicodeFormat(_In_ HWND hwnd)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_GETUNICODEFORMAT, 0, 0))); }
#endif

#undef ListView_GetBkColor
static FORCEINLINE BOOL ListView_GetBkColor(_In_ HWND hwnd)
	{ return STATIC_CAST(COLORREF)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_GETBKCOLOR, 0, 0L))); }

#undef ListView_SetBkColor
static FORCEINLINE BOOL ListView_SetBkColor(_In_ HWND hwnd, _In_ COLORREF clrBk)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_SETBKCOLOR, 0, STATIC_CAST(LPARAM)(clrBk)))); }

#undef ListView_GetImageList
static FORCEINLINE HIMAGELIST ListView_GetImageList(_In_ HWND hwnd, _In_ int iImageList)
	{ return REINTERPRET_CAST(HIMAGELIST)(SNDMSG(hwnd, LVM_GETIMAGELIST, STATIC_CAST(WPARAM)(iImageList), 0L)); }

#undef ListView_SetImageList
static FORCEINLINE HIMAGELIST ListView_SetImageList(_In_ HWND hwnd, _In_ HIMAGELIST himl, _In_ int iImageList)
	{ return REINTERPRET_CAST(HIMAGELIST)(SNDMSG(hwnd, LVM_SETIMAGELIST, STATIC_CAST(WPARAM)(iImageList), REINTERPRET_CAST(LPARAM)(himl))); }

#undef ListView_GetItemCount
static FORCEINLINE int ListView_GetItemCount(_In_ HWND hwnd)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_GETITEMCOUNT, 0, 0L))); }

#undef ListView_GetItem
static FORCEINLINE BOOL ListView_GetItem(_In_ HWND hwnd, _Out_ LVITEM *pitem)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_GETITEM, 0, REINTERPRET_CAST(LPARAM)(pitem)))); }

#undef ListView_SetItem
static FORCEINLINE BOOL ListView_SetItem(_In_ HWND hwnd, _In_ const LVITEM *pitem)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_SETITEM, 0, REINTERPRET_CAST(LPARAM)(pitem)))); }

#undef ListView_InsertItem
static FORCEINLINE BOOL ListView_InsertItem(_In_ HWND hwnd, _In_ const LVITEM *pitem)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_INSERTITEM, 0, REINTERPRET_CAST(LPARAM)(pitem)))); }

#undef ListView_DeleteItem
static FORCEINLINE BOOL ListView_DeleteItem(_In_ HWND hwnd, _In_ int iItem)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_DELETEITEM, STATIC_CAST(WPARAM)(iItem), 0L))); }

#undef ListView_DeleteAllItems
static FORCEINLINE BOOL ListView_DeleteAllItems(_In_ HWND hwnd)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_DELETEALLITEMS, 0, 0L))); }

#undef ListView_GetCallbackMask
static FORCEINLINE UINT ListView_GetCallbackMask(_In_ HWND hwnd)
	{ return STATIC_CAST(UINT)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_GETCALLBACKMASK, 0, 0))); }

#undef ListView_SetCallbackMask
static FORCEINLINE BOOL ListView_SetCallbackMask(_In_ HWND hwnd, _In_ UINT mask)
	{ return STATIC_CAST(UINT)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_SETCALLBACKMASK, STATIC_CAST(WPARAM)(mask), 0))); }

#undef ListView_GetNextItem
static FORCEINLINE UINT ListView_GetNextItem(_In_ HWND hwnd, _In_ int iStart, _In_ UINT flags)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_GETNEXTITEM, STATIC_CAST(WPARAM)(iStart), MAKELPARAM(flags, 0)))); }

#undef ListView_FindItem
static FORCEINLINE int ListView_FindItem(_In_ HWND hwnd, _In_ int iStart, _In_ const LVFINDINFO *plvfi)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_FINDITEM, STATIC_CAST(WPARAM)(iStart), REINTERPRET_CAST(LPARAM)(plvfi)))); }

#undef ListView_GetItemRect
static FORCEINLINE BOOL ListView_GetItemRect(_In_ HWND hwnd, _In_ int i, _Out_ RECT *prc, _In_ int code)
{
	if (prc) {
		prc->left = code;
	}
	return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_GETITEMRECT, STATIC_CAST(WPARAM)(i), REINTERPRET_CAST(LPARAM)(prc))));
}

#undef ListView_SetItemPosition
static FORCEINLINE BOOL ListView_SetItemPosition(_In_ HWND hwndLV, _In_ int i, _In_ int x, _In_ int y)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwndLV, LVM_SETITEMPOSITION, STATIC_CAST(WPARAM)(i), MAKELPARAM(x, y))); }

#undef ListView_GetItemPosition
static FORCEINLINE BOOL ListView_GetItemPosition(_In_ HWND hwndLV, _In_ int i, _Out_ LPPOINT ppt)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwndLV, LVM_GETITEMPOSITION, STATIC_CAST(WPARAM)(i), REINTERPRET_CAST(LPARAM)(ppt))); }

#undef ListView_GetStringWidth
static FORCEINLINE int ListView_GetStringWidth(_In_ HWND hwndLV, _In_ LPCTSTR psz)
	{ return STATIC_CAST(int)(SNDMSG(hwndLV, LVM_GETSTRINGWIDTH, 0, REINTERPRET_CAST(LPARAM)(psz))); }

#undef ListView_HitTest
static FORCEINLINE int ListView_HitTest(_In_ HWND hwndLV, _Inout_ LV_HITTESTINFO *pinfo)
	{ return STATIC_CAST(int)(SNDMSG(hwndLV, LVM_HITTEST, 0, REINTERPRET_CAST(LPARAM)(pinfo))); }

#undef ListView_HitTestEx
static FORCEINLINE int ListView_HitTestEx(_In_ HWND hwndLV, _Inout_ LV_HITTESTINFO *pinfo)
	{ return STATIC_CAST(int)(SNDMSG(hwndLV, LVM_HITTEST, -1, REINTERPRET_CAST(LPARAM)(pinfo))); }

#undef ListView_EnsureVisible
static FORCEINLINE BOOL ListView_EnsureVisible(_In_ HWND hwndLV, _In_ int i, _In_ BOOL fPartialOK)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwndLV, LVM_ENSUREVISIBLE, STATIC_CAST(WPARAM)(i), MAKELPARAM(fPartialOK, 0))); }

#undef ListView_Scroll
static FORCEINLINE BOOL ListView_Scroll(_In_ HWND hwndLV, _In_ int dx, _In_ int dy)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwndLV, LVM_SCROLL, STATIC_CAST(WPARAM)(dx), STATIC_CAST(LPARAM)(dy))); }

#undef ListView_RedrawItems
static FORCEINLINE BOOL ListView_RedrawItems(_In_ HWND hwndLV, _In_ int iFirst, _In_ int iLast)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_REDRAWITEMS, STATIC_CAST(WPARAM)(iFirst), STATIC_CAST(LPARAM)(iLast)))); }

#undef ListView_Arrange
static FORCEINLINE BOOL ListView_Arrange(_In_ HWND hwndLV, _In_ UINT code)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwndLV, LVM_ARRANGE, STATIC_CAST(WPARAM)(code), 0L)); }

#undef ListView_EditLabel
static FORCEINLINE HWND ListView_EditLabel(_In_ HWND hwndLV, _In_ int i)
	{ return REINTERPRET_CAST(HWND)(SNDMSG(hwndLV, LVM_EDITLABEL, STATIC_CAST(WPARAM)(i), 0L)); }

#undef ListView_GetEditControl
static FORCEINLINE HWND ListView_GetEditControl(_In_ HWND hwndLV)
	{ return REINTERPRET_CAST(HWND)(SNDMSG(hwndLV, LVM_GETEDITCONTROL, 0, 0L)); }

#undef ListView_GetColumn
static FORCEINLINE BOOL ListView_GetColumn(_In_ HWND hwnd, _In_ int iCol, _Inout_ LV_COLUMN *pcol)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwnd, LVM_GETCOLUMN, STATIC_CAST(WPARAM)(iCol), REINTERPRET_CAST(LPARAM)(pcol))); }

#undef ListView_SetColumn
static FORCEINLINE BOOL ListView_SetColumn(_In_ HWND hwnd, _In_ int iCol, _In_ const LPLVCOLUMN pcol)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_SETCOLUMN, STATIC_CAST(WPARAM)(iCol), REINTERPRET_CAST(LPARAM)(pcol)))); }

#undef ListView_InsertColumn
static FORCEINLINE int ListView_InsertColumn(_In_ HWND hwnd, _In_ int iCol, _In_ const LPLVCOLUMN pcol)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_INSERTCOLUMN, STATIC_CAST(WPARAM)(iCol), REINTERPRET_CAST(LPARAM)(pcol)))); }

#undef ListView_DeleteColumn
static FORCEINLINE BOOL ListView_DeleteColumn(_In_ HWND hwnd, _In_ int iCol)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_DELETECOLUMN, STATIC_CAST(WPARAM)(iCol), 0))); }

#undef ListView_GetColumnWidth
static FORCEINLINE int ListView_GetColumnWidth(_In_ HWND hwnd, _In_ int iCol)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_GETCOLUMNWIDTH, STATIC_CAST(WPARAM)(iCol), 0))); }

#undef ListView_SetColumnWidth
static FORCEINLINE BOOL ListView_SetColumnWidth(_In_ HWND hwnd, _In_ int iCol, _In_ int cx)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_SETCOLUMNWIDTH, STATIC_CAST(WPARAM)(iCol), MAKELPARAM(cx, 0)))); }

#if (_WIN32_IE >= 0x0300)
#undef ListView_GetHeader
static FORCEINLINE HWND ListView_GetHeader(_In_ HWND hwnd)
	{ return REINTERPRET_CAST(HWND)(SNDMSG(hwnd, LVM_GETHEADER, 0, 0L)); }
#endif

#if 0 /* TODO */
#define ListView_CreateDragImage(hwnd, i, lpptUpLeft) \
    (HIMAGELIST)SNDMSG((hwnd), LVM_CREATEDRAGIMAGE, (WPARAM)(int)(i), (LPARAM)(LPPOINT)(lpptUpLeft))

#define ListView_GetViewRect(hwnd, prc) \
    (BOOL)SNDMSG((hwnd), LVM_GETVIEWRECT, 0, (LPARAM)(RECT *)(prc))

#define ListView_GetTextColor(hwnd)  \
    (COLORREF)SNDMSG((hwnd), LVM_GETTEXTCOLOR, 0, 0L)

#define ListView_SetTextColor(hwnd, clrText) \
    (BOOL)SNDMSG((hwnd), LVM_SETTEXTCOLOR, 0, (LPARAM)(COLORREF)(clrText))

#define ListView_GetTextBkColor(hwnd)  \
    (COLORREF)SNDMSG((hwnd), LVM_GETTEXTBKCOLOR, 0, 0L)

#define ListView_SetTextBkColor(hwnd, clrTextBk) \
    (BOOL)SNDMSG((hwnd), LVM_SETTEXTBKCOLOR, 0, (LPARAM)(COLORREF)(clrTextBk))

#define ListView_GetTopIndex(hwndLV) \
    (int)SNDMSG((hwndLV), LVM_GETTOPINDEX, 0, 0)

#define ListView_GetCountPerPage(hwndLV) \
    (int)SNDMSG((hwndLV), LVM_GETCOUNTPERPAGE, 0, 0)

#define ListView_GetOrigin(hwndLV, ppt) \
    (BOOL)SNDMSG((hwndLV), LVM_GETORIGIN, (WPARAM)0, (LPARAM)(POINT *)(ppt))
#endif /* TODO */

#undef ListView_Update
static FORCEINLINE BOOL ListView_Update(_In_ HWND hwndLV, _In_ int iItem)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_UPDATE, STATIC_CAST(WPARAM)(iItem), 0L))); }

#undef ListView_SetItemState
static FORCEINLINE BOOL ListView_SetItemState(_In_ HWND hwndLV, _In_ int iItem, _In_ UINT state, _In_ UINT mask)
{
	LVITEM lvi;
	lvi.stateMask = mask;
	lvi.state = state;
	return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_SETITEMSTATE, STATIC_CAST(WPARAM)(iItem), REINTERPRET_CAST(LPARAM)(&lvi))));
}

#undef ListView_GetItemState
static FORCEINLINE UINT ListView_GetItemState(_In_ HWND hwndLV, _In_ int iItem, _In_ UINT mask)
	{ return STATIC_CAST(UINT)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_GETITEMSTATE, STATIC_CAST(WPARAM)(iItem), STATIC_CAST(LPARAM)(mask)))); }

#if (_WIN32_IE >= 0x0300)
#undef ListView_GetCheckState
static FORCEINLINE UINT ListView_GetCheckState(_In_ HWND hwndLV, _In_ int iItem)
	{ return (((STATIC_CAST(UINT)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_GETITEMSTATE, STATIC_CAST(WPARAM)(iItem), LVIS_STATEIMAGEMASK)))) >> 12) - 1); }
#endif

#undef ListView_GetItemText
static FORCEINLINE void ListView_GetItemText(_In_ HWND hwndLV, _In_ int iItem, _In_ int iSubItem, _Out_ LPTSTR pszText, _In_ int cchTextMax)
{
	LVITEM lvi;
	lvi.iSubItem = iSubItem;
	lvi.cchTextMax = cchTextMax;
	lvi.pszText = pszText;
	(void)SNDMSG(hwndLV, LVM_GETITEMTEXT, STATIC_CAST(WPARAM)(iItem), REINTERPRET_CAST(LPARAM)(&lvi));
}

#undef ListView_SetItemText
static FORCEINLINE void ListView_SetItemText(_In_ HWND hwndLV, _In_ int iItem, _In_ int iSubItem, _In_ LPCTSTR pszText)
{
	LVITEM lvi;
	lvi.iSubItem = iSubItem;
	lvi.pszText = CONST_CAST(LPTSTR)(pszText);
	(void)SNDMSG(hwndLV, LVM_SETITEMTEXT, STATIC_CAST(WPARAM)(iItem), REINTERPRET_CAST(LPARAM)(&lvi));
}

#undef ListView_SetItemCount
static FORCEINLINE BOOL ListView_SetItemCount(_In_ HWND hwndLV, _In_ int cItems)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_SETITEMCOUNT, STATIC_CAST(WPARAM)(cItems), 0))); }

#if (_WIN32_IE >= 0x0300)
#undef ListView_SetItemCountEx
static FORCEINLINE BOOL ListView_SetItemCountEx(_In_ HWND hwndLV, _In_ int cItems, _In_ DWORD dwFlags)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_SETITEMCOUNT, STATIC_CAST(WPARAM)(cItems), STATIC_CAST(LPARAM)(dwFlags)))); }
#endif

#undef ListView_SortItems
static FORCEINLINE BOOL ListView_SortItems(_In_ HWND hwndLV, _In_ PFNLVCOMPARE pfnCompare, _In_ LPARAM lParamSort)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_SORTITEMS, REINTERPRET_CAST(WPARAM)(pfnCompare), lParamSort))); }

#undef ListView_SetItemPosition32
static FORCEINLINE void ListView_SetItemPosition32(_In_ HWND hwndLV, _In_ int iItem, _In_ int x, _In_ int y)
{
	const POINT ptNewPos = {x, y};
	(void)SNDMSG(hwndLV, LVM_SETITEMPOSITION32, STATIC_CAST(WPARAM)(iItem), REINTERPRET_CAST(LPARAM)(&ptNewPos));
}

#undef ListView_GetSelectedCount
static FORCEINLINE UINT ListView_GetSelectedCount(_In_ HWND hwndLV)
	{ return STATIC_CAST(UINT)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_GETSELECTEDCOUNT, 0, 0L))); }

#undef ListView_GetItemSpacing
static FORCEINLINE DWORD ListView_GetItemSpacing(_In_ HWND hwndLV, BOOL fSmall)
	{ return STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_GETITEMSPACING, STATIC_CAST(WPARAM)(fSmall), 0L)); }

#undef ListView_GetISearchString
static FORCEINLINE BOOL ListView_GetISearchString(_In_ HWND hwndLV, _Out_opt_ LPTSTR lpsz)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_GETISEARCHSTRING, 0, REINTERPRET_CAST(LPARAM)(lpsz)))); }

#if (_WIN32_IE >= 0x0300)
#undef ListView_SetIconSpacing
static FORCEINLINE DWORD ListView_SetIconSpacing(_In_ HWND hwndLV, _In_ int cx, _In_ int cy)
	{ return STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_SETICONSPACING, 0, STATIC_CAST(LPARAM)(MAKELONG(cx, cy)))); }

#undef ListView_SetExtendedListViewStyle
static FORCEINLINE DWORD ListView_SetExtendedListViewStyle(_In_ HWND hwndLV, _In_ DWORD dwExStyle)
	{ return STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, STATIC_CAST(LPARAM)(dwExStyle))); }
#if (_WIN32_IE >= 0x0400)
#undef ListView_SetExtendedListViewStyleEx
static FORCEINLINE DWORD ListView_SetExtendedListViewStyleEx(_In_ HWND hwndLV, _In_ DWORD dwExMask, _In_ DWORD dwExStyle)
	{ return STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_SETEXTENDEDLISTVIEWSTYLE, STATIC_CAST(WPARAM)(dwExMask), STATIC_CAST(LPARAM)(dwExStyle))); }
#endif

#undef ListView_GetExtendedListViewStyle
static FORCEINLINE DWORD ListView_GetExtendedListViewStyle(_In_ HWND hwndLV)
	{ return STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0)); }

#undef ListView_GetSubItemRect
static FORCEINLINE BOOL ListView_GetSubItemRect(_In_ HWND hwndLV, _In_ int iItem, _In_ int iSubItem, _In_ int code, _Out_ LPRECT lpRect)
{
	if (lpRect) {
		lpRect->top = iSubItem;
		lpRect->left = code;
	}
	return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_GETSUBITEMRECT, STATIC_CAST(WPARAM)(iItem), REINTERPRET_CAST(LPARAM)(lpRect))));
}

#undef ListView_SubItemHitTest
static FORCEINLINE int ListView_SubItemHitTest(_In_ HWND hwndLV, LPLVHITTESTINFO plvhti)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_SUBITEMHITTEST, 0, REINTERPRET_CAST(LPARAM)(plvhti)))); }
#undef ListView_SubItemHitTestEx
static FORCEINLINE int ListView_SubItemHitTestEx(_In_ HWND hwndLV, LPLVHITTESTINFO plvhti)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_SUBITEMHITTEST, STATIC_CAST(WPARAM)(-1), REINTERPRET_CAST(LPARAM)(plvhti)))); }

#if 0 /* TODO */
#define ListView_SetColumnOrderArray(hwnd, iCount, pi) \
        (BOOL)SNDMSG((hwnd), LVM_SETCOLUMNORDERARRAY, (WPARAM)(iCount), (LPARAM)(LPINT)(pi))

#define ListView_GetColumnOrderArray(hwnd, iCount, pi) \
        (BOOL)SNDMSG((hwnd), LVM_GETCOLUMNORDERARRAY, (WPARAM)(iCount), (LPARAM)(LPINT)(pi))

#define ListView_SetHotItem(hwnd, i) \
        (int)SNDMSG((hwnd), LVM_SETHOTITEM, (WPARAM)(i), 0)

#define ListView_GetHotItem(hwnd) \
        (int)SNDMSG((hwnd), LVM_GETHOTITEM, 0, 0)

#define ListView_SetHotCursor(hwnd, hcur) \
        (HCURSOR)SNDMSG((hwnd), LVM_SETHOTCURSOR, 0, (LPARAM)(hcur))

#define ListView_GetHotCursor(hwnd) \
        (HCURSOR)SNDMSG((hwnd), LVM_GETHOTCURSOR, 0, 0)

#define ListView_ApproximateViewRect(hwnd, iWidth, iHeight, iCount) \
        (DWORD)SNDMSG((hwnd), LVM_APPROXIMATEVIEWRECT, (WPARAM)(iCount), MAKELPARAM(iWidth, iHeight))
#endif /* TODO */
#endif /* _WIN32_IE >= 0x0300 */

#if (_WIN32_IE >= 0x0400)

#if 0 /* TODO */
#define ListView_SetWorkAreas(hwnd, nWorkAreas, prc) \
    (BOOL)SNDMSG((hwnd), LVM_SETWORKAREAS, (WPARAM)(int)(nWorkAreas), (LPARAM)(RECT *)(prc))

#define ListView_GetWorkAreas(hwnd, nWorkAreas, prc) \
    (BOOL)SNDMSG((hwnd), LVM_GETWORKAREAS, (WPARAM)(int)(nWorkAreas), (LPARAM)(RECT *)(prc))

#define ListView_GetNumberOfWorkAreas(hwnd, pnWorkAreas) \
    (BOOL)SNDMSG((hwnd), LVM_GETNUMBEROFWORKAREAS, 0, (LPARAM)(UINT *)(pnWorkAreas))

#define ListView_GetSelectionMark(hwnd) \
    (int)SNDMSG((hwnd), LVM_GETSELECTIONMARK, 0, 0)

#define ListView_SetSelectionMark(hwnd, i) \
    (int)SNDMSG((hwnd), LVM_SETSELECTIONMARK, 0, (LPARAM)(i))

#define ListView_SetHoverTime(hwndLV, dwHoverTimeMs)\
        (DWORD)SNDMSG((hwndLV), LVM_SETHOVERTIME, 0, (LPARAM)(dwHoverTimeMs))

#define ListView_GetHoverTime(hwndLV)\
        (DWORD)SNDMSG((hwndLV), LVM_GETHOVERTIME, 0, 0)

#define ListView_SetToolTips(hwndLV, hwndNewHwnd)\
        (HWND)SNDMSG((hwndLV), LVM_SETTOOLTIPS, (WPARAM)(hwndNewHwnd), 0)

#define ListView_GetToolTips(hwndLV)\
        (HWND)SNDMSG((hwndLV), LVM_GETTOOLTIPS, 0, 0)

#define ListView_SortItemsEx(hwndLV, _pfnCompare, _lPrm) \
  (BOOL)SNDMSG((hwndLV), LVM_SORTITEMSEX, (WPARAM)(LPARAM)(_lPrm), (LPARAM)(PFNLVCOMPARE)(_pfnCompare))
#endif /* TODO */

#if (_WIN32_WINNT >= 0x0501)
#if 0 /* TODO */
#define ListView_SetSelectedColumn(hwnd, iCol) \
    SNDMSG((hwnd), LVM_SETSELECTEDCOLUMN, (WPARAM)(iCol), 0)

#define ListView_SetView(hwnd, iView) \
    (DWORD)SNDMSG((hwnd), LVM_SETVIEW, (WPARAM)(DWORD)(iView), 0)

#define ListView_GetView(hwnd) \
    (DWORD)SNDMSG((hwnd), LVM_GETVIEW, 0, 0)

#define ListView_InsertGroup(hwnd, index, pgrp) \
    SNDMSG((hwnd), LVM_INSERTGROUP, (WPARAM)(index), (LPARAM)(pgrp))

#define ListView_SetGroupInfo(hwnd, iGroupId, pgrp) \
    SNDMSG((hwnd), LVM_SETGROUPINFO, (WPARAM)(iGroupId), (LPARAM)(pgrp))

#define ListView_GetGroupInfo(hwnd, iGroupId, pgrp) \
    SNDMSG((hwnd), LVM_GETGROUPINFO, (WPARAM)(iGroupId), (LPARAM)(pgrp))

#define ListView_RemoveGroup(hwnd, iGroupId) \
    SNDMSG((hwnd), LVM_REMOVEGROUP, (WPARAM)(iGroupId), 0)

#define ListView_MoveGroup(hwnd, iGroupId, toIndex) \
    SNDMSG((hwnd), LVM_MOVEGROUP, (WPARAM)(iGroupId), (LPARAM)(toIndex))

#define ListView_GetGroupCount(hwnd) \
    SNDMSG((hwnd), LVM_GETGROUPCOUNT, (WPARAM)0, (LPARAM)0)

#define ListView_GetGroupInfoByIndex(hwnd, iIndex, pgrp) \
    SNDMSG((hwnd), LVM_GETGROUPINFOBYINDEX, (WPARAM)(iIndex), (LPARAM)(pgrp))

#define ListView_MoveItemToGroup(hwnd, idItemFrom, idGroupTo) \
    SNDMSG((hwnd), LVM_MOVEITEMTOGROUP, (WPARAM)(idItemFrom), (LPARAM)(idGroupTo))
#endif /* TODO */

#undef ListView_GetGroupRect
static FORCEINLINE BOOL ListView_GetGroupRect(_In_ HWND hwnd, _In_ int iGroupId, _In_ LONG type, _Inout_ RECT *prc)
{
	if (prc) {
		prc->top = type;
	}
	return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_GETGROUPRECT, STATIC_CAST(WPARAM)(iGroupId), REINTERPRET_CAST(LPARAM)(prc))));
}

#if 0 /* TODO */
#define ListView_SetGroupMetrics(hwnd, pGroupMetrics) \
    SNDMSG((hwnd), LVM_SETGROUPMETRICS, 0, (LPARAM)(pGroupMetrics))

#define ListView_GetGroupMetrics(hwnd, pGroupMetrics) \
    SNDMSG((hwnd), LVM_GETGROUPMETRICS, 0, (LPARAM)(pGroupMetrics))

#define ListView_EnableGroupView(hwnd, fEnable) \
    SNDMSG((hwnd), LVM_ENABLEGROUPVIEW, (WPARAM)(fEnable), 0)

#define ListView_SortGroups(hwnd, _pfnGroupCompate, _plv) \
    SNDMSG((hwnd), LVM_SORTGROUPS, (WPARAM)(_pfnGroupCompate), (LPARAM)(_plv))

#define ListView_InsertGroupSorted(hwnd, structInsert) \
    SNDMSG((hwnd), LVM_INSERTGROUPSORTED, (WPARAM)(structInsert), 0)

#define ListView_RemoveAllGroups(hwnd) \
    SNDMSG((hwnd), LVM_REMOVEALLGROUPS, 0, 0)

#define ListView_HasGroup(hwnd, dwGroupId) \
    SNDMSG((hwnd), LVM_HASGROUP, dwGroupId, 0)
#endif /* TODO */

#undef ListView_SetGroupState
static FORCEINLINE LRESULT ListView_SetGroupState(_In_ HWND hwnd, _In_ UINT dwGroupId, _In_ UINT dwMask, _In_ UINT dwState)
{
	LVGROUP lvg;
	lvg.cbSize = sizeof(lvg);
	lvg.mask = LVGF_STATE;
	lvg.stateMask = dwMask;
	lvg.state = dwState;
	return SNDMSG(hwnd, LVM_SETGROUPINFO, STATIC_CAST(WPARAM)(dwGroupId), REINTERPRET_CAST(LPARAM)(&lvg));
}
#undef ListView_GetGroupState
static FORCEINLINE UINT ListView_GetGroupState(_In_ HWND hwnd, _In_ UINT dwGroupId, _In_ UINT dwMask)
	{ return STATIC_CAST(UINT)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_GETGROUPSTATE, STATIC_CAST(WPARAM)(dwGroupId), STATIC_CAST(LPARAM)(dwMask)))); }

#if 0 /* TODO */
#define ListView_GetFocusedGroup(hwnd) \
    SNDMSG((hwnd), LVM_GETFOCUSEDGROUP, 0, 0)

#define ListView_SetTileViewInfo(hwnd, ptvi) \
    SNDMSG((hwnd), LVM_SETTILEVIEWINFO, 0, (LPARAM)(ptvi))

#define ListView_GetTileViewInfo(hwnd, ptvi) \
    SNDMSG((hwnd), LVM_GETTILEVIEWINFO, 0, (LPARAM)(ptvi))

#define ListView_SetTileInfo(hwnd, pti) \
    SNDMSG((hwnd), LVM_SETTILEINFO, 0, (LPARAM)(pti))

#define ListView_GetTileInfo(hwnd, pti) \
    SNDMSG((hwnd), LVM_GETTILEINFO, 0, (LPARAM)(pti))

#define ListView_SetInsertMark(hwnd, lvim) \
    (BOOL)SNDMSG((hwnd), LVM_SETINSERTMARK, (WPARAM) 0, (LPARAM) (lvim))

#define ListView_GetInsertMark(hwnd, lvim) \
    (BOOL)SNDMSG((hwnd), LVM_GETINSERTMARK, (WPARAM) 0, (LPARAM) (lvim))

#define ListView_InsertMarkHitTest(hwnd, point, lvim) \
    (int)SNDMSG((hwnd), LVM_INSERTMARKHITTEST, (WPARAM)(LPPOINT)(point), (LPARAM)(LPLVINSERTMARK)(lvim))

#define ListView_GetInsertMarkRect(hwnd, rc) \
    (int)SNDMSG((hwnd), LVM_GETINSERTMARKRECT, (WPARAM)0, (LPARAM)(LPRECT)(rc))

#define ListView_SetInsertMarkColor(hwnd, color) \
    (COLORREF)SNDMSG((hwnd), LVM_SETINSERTMARKCOLOR, (WPARAM)0, (LPARAM)(COLORREF)(color))

#define ListView_GetInsertMarkColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), LVM_GETINSERTMARKCOLOR, (WPARAM)0, (LPARAM)0)

#define ListView_SetInfoTip(hwndLV, plvInfoTip)\
        (BOOL)SNDMSG((hwndLV), LVM_SETINFOTIP, (WPARAM)0, (LPARAM)(plvInfoTip))

#define ListView_GetSelectedColumn(hwnd) \
    (UINT)SNDMSG((hwnd), LVM_GETSELECTEDCOLUMN, 0, 0)

#define ListView_IsGroupViewEnabled(hwnd) \
    (BOOL)SNDMSG((hwnd), LVM_ISGROUPVIEWENABLED, 0, 0)

#define ListView_GetOutlineColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), LVM_GETOUTLINECOLOR, 0, 0)

#define ListView_SetOutlineColor(hwnd, color) \
    (COLORREF)SNDMSG((hwnd), LVM_SETOUTLINECOLOR, (WPARAM)0, (LPARAM)(COLORREF)(color))

#define ListView_CancelEditLabel(hwnd) \
    (VOID)SNDMSG((hwnd), LVM_CANCELEDITLABEL, (WPARAM)0, (LPARAM)0)

#define ListView_MapIndexToID(hwnd, index) \
    (UINT)SNDMSG((hwnd), LVM_MAPINDEXTOID, (WPARAM)(index), (LPARAM)0)

#define ListView_MapIDToIndex(hwnd, id) \
    (UINT)SNDMSG((hwnd), LVM_MAPIDTOINDEX, (WPARAM)(id), (LPARAM)0)

#define ListView_IsItemVisible(hwnd, index) \
    (UINT)SNDMSG((hwnd), LVM_ISITEMVISIBLE, (WPARAM)(index), (LPARAM)0)
#endif /* TODO */

#if _WIN32_WINNT >= 0x0600
#if 0 /* TODO */
#define ListView_SetGroupHeaderImageList(hwnd, himl) \
    (HIMAGELIST)SNDMSG((hwnd), LVM_SETIMAGELIST, (WPARAM)LVSIL_GROUPHEADER, (LPARAM)(HIMAGELIST)(himl))

#define ListView_GetGroupHeaderImageList(hwnd) \
    (HIMAGELIST)SNDMSG((hwnd), LVM_GETIMAGELIST, (WPARAM)LVSIL_GROUPHEADER, 0L)

#define ListView_GetEmptyText(hwnd, pszText, cchText) \
    (BOOL)SNDMSG((hwnd), LVM_GETEMPTYTEXT, (WPARAM)(cchText), (LPARAM)(pszText))

#define ListView_GetFooterRect(hwnd, prc) \
    (BOOL)SNDMSG((hwnd), LVM_GETFOOTERRECT, (WPARAM)(0), (LPARAM)(prc))

#define ListView_GetFooterInfo(hwnd, plvfi) \
    (BOOL)SNDMSG((hwnd), LVM_GETFOOTERINFO, (WPARAM)(0), (LPARAM)(plvfi))

#define ListView_GetFooterItemRect(hwnd, iItem, prc) \
    (BOOL)SNDMSG((hwnd), LVM_GETFOOTERITEMRECT, (WPARAM)(iItem), (LPARAM)(prc))

#define ListView_GetFooterItem(hwnd, iItem, pfi) \
    (BOOL)SNDMSG((hwnd), LVM_GETFOOTERITEM, (WPARAM)(iItem), (LPARAM)(pfi))
#endif /* TODO */

#undef ListView_GetItemIndexRect
static FORCEINLINE BOOL ListView_GetItemIndexRect(_In_ HWND hwnd, _In_ LVITEMINDEX *plvii, _In_ LONG iSubItem, _In_ LONG code, _Inout_ LPRECT prc)
{
	if (prc) {
		prc->top = iSubItem;
		prc->left = code;
	}
	return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, LVM_GETITEMINDEXRECT, REINTERPRET_CAST(WPARAM)(plvii), REINTERPRET_CAST(LPARAM)(prc))));
}

#undef ListView_SetItemIndexState
static FORCEINLINE HRESULT ListView_SetItemIndexState(_In_ HWND hwndLV, _In_ LVITEMINDEX *plvii, _In_ UINT state, _In_ UINT mask)
{
	LVITEM lvi;
	lvi.stateMask = mask;
	lvi.state = state;
	return STATIC_CAST(HRESULT)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_SETITEMINDEXSTATE, REINTERPRET_CAST(WPARAM)(plvii), REINTERPRET_CAST(LPARAM)(&lvi))));
}

#if 0 /* TODO */
#define ListView_GetNextItemIndex(hwnd, plvii, flags) \
    (BOOL)SNDMSG((hwnd), LVM_GETNEXTITEMINDEX, (WPARAM)(LVITEMINDEX*)(plvii), MAKELPARAM((flags), 0))
#endif /* TODO */

#endif /* _WIN32_WINNT >= 0x0600 */

#endif /* _WIN32_WINNT >= 0x0501 */

#undef ListView_SetBkImage
static FORCEINLINE BOOL ListView_SetBkImage(_In_ HWND hwndLV, _In_ LPLVBKIMAGE plvbki)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_SETBKIMAGE, 0, REINTERPRET_CAST(LPARAM)(plvbki)))); }

#undef ListView_GetBkImage
static FORCEINLINE BOOL ListView_GetBkImage(_In_ HWND hwndLV, _Inout_ LPLVBKIMAGE plvbki)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndLV, LVM_GETBKIMAGE, 0, REINTERPRET_CAST(LPARAM)(plvbki)))); }

#endif /* _WIN32_IE >= 0x0400 */

#endif /* NOLISTVIEW */

//====== TREEVIEW CONTROL =====================================================
#ifndef NOTREEVIEW

#undef TreeView_InsertItem
static FORCEINLINE HTREEITEM TreeView_InsertItem(_In_ HWND hwnd, _In_ LPTV_INSERTSTRUCT lpis)
	{ return REINTERPRET_CAST(HTREEITEM)(SNDMSG(hwnd, TVM_INSERTITEM, 0, REINTERPRET_CAST(LPARAM)(lpis))); }

#undef TreeView_DeleteItem
static FORCEINLINE BOOL TreeView_DeleteItem(_In_ HWND hwnd, _In_ HTREEITEM hitem)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwnd, TVM_DELETEITEM, 0, REINTERPRET_CAST(LPARAM)(hitem))); }

#undef TreeView_DeleteAllItems
static FORCEINLINE BOOL TreeView_DeleteAllItems(_In_ HWND hwnd)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwnd, TVM_DELETEITEM, 0, REINTERPRET_CAST(LPARAM)(TVI_ROOT))); }

#undef TreeView_Expand
static FORCEINLINE BOOL TreeView_Expand(_In_ HWND hwnd, _In_ HTREEITEM hitem, _In_ UINT code)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwnd, TVM_EXPAND, STATIC_CAST(WPARAM)(code), REINTERPRET_CAST(LPARAM)(hitem))); }

#undef TreeView_GetItemRect
static FORCEINLINE BOOL TreeView_GetItemRect(_In_ HWND hwnd, _In_ HTREEITEM hitem, _Out_ RECT *prc, _In_ BOOL fItemRect)
{
	(*(HTREEITEM*)prc) = hitem;
	return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, TVM_GETITEMRECT, STATIC_CAST(WPARAM)(fItemRect), REINTERPRET_CAST(LPARAM)(prc))));
}

#undef TreeView_GetCount
static FORCEINLINE UINT TreeView_GetCount(_In_ HWND hwnd)
	{ return STATIC_CAST(UINT)(SNDMSG(hwnd, TVM_GETCOUNT, 0, 0)); }

#undef TreeView_GetIndent
static FORCEINLINE UINT TreeView_GetIndent(_In_ HWND hwnd)
	{ return STATIC_CAST(UINT)(SNDMSG(hwnd, TVM_GETINDENT, 0, 0)); }

#undef TreeView_SetIndent
static FORCEINLINE BOOL TreeView_SetIndent(_In_ HWND hwnd, _In_ UINT indent)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwnd, TVM_SETINDENT, STATIC_CAST(WPARAM)(indent), 0)); }

#undef TreeView_GetImageList
static FORCEINLINE HIMAGELIST TreeView_GetImageList(_In_ HWND hwnd, _In_ int iImage)
	{ return REINTERPRET_CAST(HIMAGELIST)(SNDMSG(hwnd, TVM_GETIMAGELIST, iImage, 0)); }

#undef TreeView_SetImageList
static FORCEINLINE HIMAGELIST TreeView_SetImageList(_In_ HWND hwnd, _In_ HIMAGELIST himl, _In_ int iImage)
	{ return REINTERPRET_CAST(HIMAGELIST)(SNDMSG(hwnd, TVM_SETIMAGELIST, iImage, REINTERPRET_CAST(LPARAM)(himl))); }

#undef TreeView_GetNextItem
static FORCEINLINE HTREEITEM TreeView_GetNextItem(_In_ HWND hwnd, _In_ HTREEITEM hitem, _In_ UINT code)
	{ return REINTERPRET_CAST(HTREEITEM)(SNDMSG(hwnd, TVM_GETNEXTITEM, STATIC_CAST(WPARAM)(code), REINTERPRET_CAST(LPARAM)(hitem))); }

#undef TreeView_Select
static FORCEINLINE BOOL TreeView_Select(_In_ HWND hwnd, _In_ HTREEITEM hitem, _In_ UINT code)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwnd, TVM_SELECTITEM, STATIC_CAST(WPARAM)(code), REINTERPRET_CAST(LPARAM)(hitem))); }

#if 0 /* TODO */
#define TreeView_GetItem(hwnd, pitem) \
    (BOOL)SNDMSG((hwnd), TVM_GETITEM, 0, (LPARAM)(TV_ITEM *)(pitem))

#define TreeView_SetItem(hwnd, pitem) \
    (BOOL)SNDMSG((hwnd), TVM_SETITEM, 0, (LPARAM)(const TV_ITEM *)(pitem))

#define TreeView_EditLabel(hwnd, hitem) \
    (HWND)SNDMSG((hwnd), TVM_EDITLABEL, 0, (LPARAM)(HTREEITEM)(hitem))

#define TreeView_GetEditControl(hwnd) \
    (HWND)SNDMSG((hwnd), TVM_GETEDITCONTROL, 0, 0)

#define TreeView_GetVisibleCount(hwnd) \
    (UINT)SNDMSG((hwnd), TVM_GETVISIBLECOUNT, 0, 0)

#define TreeView_HitTest(hwnd, lpht) \
    (HTREEITEM)SNDMSG((hwnd), TVM_HITTEST, 0, (LPARAM)(LPTV_HITTESTINFO)(lpht))

#define TreeView_CreateDragImage(hwnd, hitem) \
    (HIMAGELIST)SNDMSG((hwnd), TVM_CREATEDRAGIMAGE, 0, (LPARAM)(HTREEITEM)(hitem))

#define TreeView_SortChildren(hwnd, hitem, recurse) \
    (BOOL)SNDMSG((hwnd), TVM_SORTCHILDREN, (WPARAM)(recurse), (LPARAM)(HTREEITEM)(hitem))

#define TreeView_EnsureVisible(hwnd, hitem) \
    (BOOL)SNDMSG((hwnd), TVM_ENSUREVISIBLE, 0, (LPARAM)(HTREEITEM)(hitem))

#define TreeView_SortChildrenCB(hwnd, psort, recurse) \
    (BOOL)SNDMSG((hwnd), TVM_SORTCHILDRENCB, (WPARAM)(recurse), \
    (LPARAM)(LPTV_SORTCB)(psort))

#define TreeView_EndEditLabelNow(hwnd, fCancel) \
    (BOOL)SNDMSG((hwnd), TVM_ENDEDITLABELNOW, (WPARAM)(fCancel), 0)

#if (_WIN32_IE >= 0x0300)
#define TreeView_SetToolTips(hwnd,  hwndTT) \
    (HWND)SNDMSG((hwnd), TVM_SETTOOLTIPS, (WPARAM)(hwndTT), 0)
#define TreeView_GetToolTips(hwnd) \
    (HWND)SNDMSG((hwnd), TVM_GETTOOLTIPS, 0, 0)
#endif /* _WIN32_IE >= 0x0300 */

#define TreeView_GetISearchString(hwndTV, lpsz) \
        (BOOL)SNDMSG((hwndTV), TVM_GETISEARCHSTRING, 0, (LPARAM)(LPTSTR)(lpsz))

#if (_WIN32_IE >= 0x0400)
#define TreeView_SetInsertMark(hwnd, hItem, fAfter) \
        (BOOL)SNDMSG((hwnd), TVM_SETINSERTMARK, (WPARAM) (fAfter), (LPARAM) (hItem))

#define TreeView_SetUnicodeFormat(hwnd, fUnicode)  \
    (BOOL)SNDMSG((hwnd), TVM_SETUNICODEFORMAT, (WPARAM)(fUnicode), 0)

#define TreeView_GetUnicodeFormat(hwnd)  \
    (BOOL)SNDMSG((hwnd), TVM_GETUNICODEFORMAT, 0, 0)
#endif /* _WIN32_IE >= 0x0400 */

#if (_WIN32_IE >= 0x0400)
#define TreeView_SetItemHeight(hwnd,  iHeight) \
    (int)SNDMSG((hwnd), TVM_SETITEMHEIGHT, (WPARAM)(iHeight), 0)
#define TreeView_GetItemHeight(hwnd) \
    (int)SNDMSG((hwnd), TVM_GETITEMHEIGHT, 0, 0)

#define TreeView_SetBkColor(hwnd, clr) \
    (COLORREF)SNDMSG((hwnd), TVM_SETBKCOLOR, 0, (LPARAM)(clr))

#define TreeView_SetTextColor(hwnd, clr) \
    (COLORREF)SNDMSG((hwnd), TVM_SETTEXTCOLOR, 0, (LPARAM)(clr))

#define TreeView_GetBkColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), TVM_GETBKCOLOR, 0, 0)

#define TreeView_GetTextColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), TVM_GETTEXTCOLOR, 0, 0)

#define TreeView_SetScrollTime(hwnd, uTime) \
    (UINT)SNDMSG((hwnd), TVM_SETSCROLLTIME, uTime, 0)

#define TreeView_GetScrollTime(hwnd) \
    (UINT)SNDMSG((hwnd), TVM_GETSCROLLTIME, 0, 0)


#define TreeView_SetInsertMarkColor(hwnd, clr) \
    (COLORREF)SNDMSG((hwnd), TVM_SETINSERTMARKCOLOR, 0, (LPARAM)(clr))
#define TreeView_GetInsertMarkColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), TVM_GETINSERTMARKCOLOR, 0, 0)
#endif  /* _WIN32_IE >= 0x0400 */
#endif /* TODO */

#if (_WIN32_IE >= 0x0500)
#undef TreeView_SetItemState
static FORCEINLINE UINT TreeView_SetItemState(_In_ HWND hwndTV, _In_ HTREEITEM hItem, _In_ UINT state, _In_ UINT stateMask)
{
	TVITEM tvi;
	tvi.mask = TVIF_STATE;
	tvi.hItem = hItem;
	tvi.stateMask = stateMask;
	tvi.state = state;
	return STATIC_CAST(UINT)(STATIC_CAST(DWORD)(SNDMSG(hwndTV, TVM_SETITEM, 0, REINTERPRET_CAST(LPARAM)(&tvi))));
}

#if 0 /* TODO */
#define TreeView_SetCheckStates(hwndTV, hti, fCheck) \
  TreeView_SetItemState(hwndTV, hti, INDEXTOSTATEIMAGEMASK((fCheck)?2:1), TVIS_STATEIMAGEMASK)

#define TreeView_GetItemState(hwndTV, hti, mask) \
   (UINT)SNDMSG((hwndTV), TVM_GETITEMSTATE, (WPARAM)(hti), (LPARAM)(mask))

#define TreeView_GetCheckState(hwndTV, hti) \
   ((((UINT)(SNDMSG((hwndTV), TVM_GETITEMSTATE, (WPARAM)(hti), TVIS_STATEIMAGEMASK))) >> 12) -1)

#define TreeView_SetLineColor(hwnd, clr) \
    (COLORREF)SNDMSG((hwnd), TVM_SETLINECOLOR, 0, (LPARAM)(clr))

#define TreeView_GetLineColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), TVM_GETLINECOLOR, 0, 0)
#endif /* TODO */

#endif /* _WIN32_IE >= 0x0500 */

#if (_WIN32_WINNT >= 0x0501)
#if 0 /* TODO */
#define TreeView_MapAccIDToHTREEITEM(hwnd, id) \
    (HTREEITEM)SNDMSG((hwnd), TVM_MAPACCIDTOHTREEITEM, id, 0)

#define TreeView_MapHTREEITEMToAccID(hwnd, htreeitem) \
    (UINT)SNDMSG((hwnd), TVM_MAPHTREEITEMTOACCID, (WPARAM)(htreeitem), 0)

#define TreeView_SetExtendedStyle(hwnd, dw, mask) \
    (DWORD)SNDMSG((hwnd), TVM_SETEXTENDEDSTYLE, mask, dw)

#define TreeView_GetExtendedStyle(hwnd) \
    (DWORD)SNDMSG((hwnd), TVM_GETEXTENDEDSTYLE, 0, 0)

#define TreeView_SetAutoScrollInfo(hwnd, uPixPerSec, uUpdateTime) \
    SNDMSG((hwnd), TVM_SETAUTOSCROLLINFO, (WPARAM)(uPixPerSec), (LPARAM)(uUpdateTime))
#endif /* TODO */
#endif /* _WIN32_WINNT >= 0x0501 */

#if (_WIN32_WINNT >= 0x0600)
#if 0 /* TODO */
#define TreeView_GetSelectedCount(hwnd) \
    (DWORD)SNDMSG((hwnd), TVM_GETSELECTEDCOUNT, 0, 0)

#define TreeView_ShowInfoTip(hwnd, hitem) \
    (DWORD)SNDMSG((hwnd), TVM_SHOWINFOTIP, 0, (LPARAM)(hitem))
#endif /* TODO */

#undef TreeView_GetItemPartRect
static FORCEINLINE BOOL TreeView_GetItemPartRect(_In_ HWND hwnd, _In_ HTREEITEM hitem, _Out_ RECT *prc, _In_ TVITEMPART partid)
{
	TVGETITEMPARTRECTINFO tvgipri;
	tvgipri.hti = hitem;
	tvgipri.prc = prc;
	tvgipri.partID = partid;
	return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, TVM_GETITEMPARTRECT, 0, REINTERPRET_CAST(LPARAM)(&tvgipri))));
}

#endif /* _WIN32_WINNT >= 0x0600 */

#endif /* NOTREEVIEW */

//====== TAB CONTROL ==========================================================
#ifndef NOTABCONTROL

#if 0 /* TODO */
#define TabCtrl_GetImageList(hwnd) \
    (HIMAGELIST)SNDMSG((hwnd), TCM_GETIMAGELIST, 0, 0L)

#define TabCtrl_SetImageList(hwnd, himl) \
    (HIMAGELIST)SNDMSG((hwnd), TCM_SETIMAGELIST, 0, (LPARAM)(HIMAGELIST)(himl))

#define TabCtrl_GetItemCount(hwnd) \
    (int)SNDMSG((hwnd), TCM_GETITEMCOUNT, 0, 0L)

#define TabCtrl_GetItem(hwnd, iItem, pitem) \
    (BOOL)SNDMSG((hwnd), TCM_GETITEM, (WPARAM)(int)(iItem), (LPARAM)(TC_ITEM *)(pitem))

#define TabCtrl_SetItem(hwnd, iItem, pitem) \
    (BOOL)SNDMSG((hwnd), TCM_SETITEM, (WPARAM)(int)(iItem), (LPARAM)(TC_ITEM *)(pitem))
#endif

#undef TabCtrl_InsertItem
static FORCEINLINE int TabCtrl_InsertItem(_In_ HWND hwnd, _In_ int iItem, _In_ const TC_ITEM *pitem)
	{ return STATIC_CAST(int)(SNDMSG(hwnd, TCM_INSERTITEM, STATIC_CAST(WPARAM)(iItem), REINTERPRET_CAST(LPARAM)(pitem))); }

#undef TabCtrl_DeleteItem
static FORCEINLINE BOOL TabCtrl_DeleteItem(_In_ HWND hwnd, _In_ int i)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwnd, TCM_DELETEITEM, STATIC_CAST(WPARAM)(i), 0L)); }

#undef TabCtrl_DeleteAllItems
static FORCEINLINE BOOL TabCtrl_DeleteAllItems(_In_ HWND hwnd)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwnd, TCM_DELETEALLITEMS, 0, 0L)); }

#undef TabCtrl_GetItemRect
static FORCEINLINE BOOL TabCtrl_GetItemRect(_In_ HWND hwnd, _In_ int i, _Out_ LPRECT prc)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwnd, TCM_GETITEMRECT, STATIC_CAST(WPARAM)(i), REINTERPRET_CAST(LPARAM)(prc))); }

#undef TabCtrl_GetCurSel
static FORCEINLINE int TabCtrl_GetCurSel(_In_ HWND hwnd)
	{ return STATIC_CAST(int)(SNDMSG(hwnd, TCM_GETCURSEL, 0, 0)); }

#undef TabCtrl_SetCurSel
static FORCEINLINE int TabCtrl_SetCurSel(_In_ HWND hwnd, _In_ int i)
	{ return STATIC_CAST(int)(SNDMSG(hwnd, TCM_SETCURSEL, STATIC_CAST(WPARAM)(i), 0)); }

#undef TabCtrl_HitTest
static FORCEINLINE int TabCtrl_HitTest(_In_ HWND hwndTC, _In_ TC_HITTESTINFO *pinfo)
	{ return STATIC_CAST(int)(SNDMSG(hwndTC, TCM_HITTEST, 0, REINTERPRET_CAST(LPARAM)(pinfo))); }

#undef TabCtrl_SetItemExtra
static FORCEINLINE BOOL TabCtrl_SetItemExtra(_In_ HWND hwndTC, _In_ DWORD cb)
	{ return STATIC_CAST(BOOL)(SNDMSG((hwndTC), TCM_SETITEMEXTRA, STATIC_CAST(WPARAM)(cb), 0L)); }

#undef TabCtrl_AdjustRect
static FORCEINLINE int TabCtrl_AdjustRect(_In_ HWND hwnd, _In_ BOOL bLarger, _Inout_ LPRECT prc)
	{ return STATIC_CAST(int)(SNDMSG(hwnd, TCM_ADJUSTRECT, STATIC_CAST(WPARAM)(bLarger), REINTERPRET_CAST(LPARAM)(prc))); }

#undef TabCtrl_SetItemSize
static FORCEINLINE DWORD TabCtrl_SetItemSize(_In_ HWND hwnd, _In_ int x, _In_ int y)
	{ return STATIC_CAST(DWORD)(SNDMSG(hwnd, TCM_SETITEMSIZE, 0, MAKELPARAM(x,y))); }

#undef TabCtrl_RemoveImage
static FORCEINLINE void TabCtrl_RemoveImage(_In_ HWND hwnd, _In_ int i)
	{ (void)SNDMSG(hwnd, TCM_REMOVEIMAGE, i, 0L); }

#undef TabCtrl_SetPadding
static FORCEINLINE void TabCtrl_SetPadding(_In_ HWND hwnd, _In_ int cx, _In_ int cy)
	{ (void)SNDMSG(hwnd, TCM_SETPADDING, 0, MAKELPARAM(cx, cy)); }

#undef TabCtrl_GetRowCount
static FORCEINLINE int TabCtrl_GetRowCount(_In_ HWND hwnd)
	{ return STATIC_CAST(int)(SNDMSG(hwnd, TCM_GETROWCOUNT, 0, 0L)); }

#undef TabCtrl_GetToolTips
static FORCEINLINE HWND TabCtrl_GetToolTips(_In_ HWND hwnd)
	{ return REINTERPRET_CAST(HWND)(SNDMSG(hwnd, TCM_GETTOOLTIPS, 0, 0L)); }

#undef TabCtrl_SetToolTips
static FORCEINLINE void TabCtrl_SetToolTips(_In_ HWND hwnd, _In_ HWND hwndTT)
	{ (void)SNDMSG(hwnd, TCM_SETTOOLTIPS, REINTERPRET_CAST(WPARAM)(hwndTT), 0L); }

#undef TabCtrl_GetCurFocus
static FORCEINLINE int TabCtrl_GetCurFocus(_In_ HWND hwnd)
	{ return STATIC_CAST(int)(SNDMSG(hwnd, TCM_GETCURFOCUS, 0, 0)); }

#undef TabCtrl_SetCurFocus
static FORCEINLINE void TabCtrl_SetCurFocus(_In_ HWND hwnd, _In_ int i)
	{ SNDMSG(hwnd, TCM_SETCURFOCUS, i, 0); }

#if (_WIN32_IE >= 0x0300)
#undef TabCtrl_SetMinTabWidth
static FORCEINLINE int TabCtrl_SetMinTabWidth(_In_ HWND hwnd, _In_ int x)
	{ return STATIC_CAST(int)(SNDMSG(hwnd, TCM_SETMINTABWIDTH, 0, x)); }

#undef TabCtrl_DeselectAll
static FORCEINLINE void TabCtrl_DeselectAll(_In_ HWND hwnd, _In_ BOOL fExcludeFocus)
	{ (void)SNDMSG(hwnd, TCM_DESELECTALL, fExcludeFocus, 0); }
#endif /* _WIN32_IE >= 0x0300 */

#if (_WIN32_IE >= 0x0400)
#undef TabCtrl_HighlightItem
static FORCEINLINE BOOL TabCtrl_HighlightItem(_In_ HWND hwnd, _In_ int i, _In_ BOOL fHighlight)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwnd, TCM_HIGHLIGHTITEM, STATIC_CAST(WPARAM)(i), MAKELONG(fHighlight, 0))); }

#undef TabCtrl_SetExtendedStyle
static FORCEINLINE DWORD TabCtrl_SetExtendedStyle(_In_ HWND hwnd, _In_ DWORD dw)
	{ return STATIC_CAST(DWORD)(SNDMSG(hwnd, TCM_SETEXTENDEDSTYLE, 0, dw)); }

#undef TabCtrl_GetExtendedStyle
static FORCEINLINE DWORD TabCtrl_GetExtendedStyle(_In_ HWND hwnd)
	{ return STATIC_CAST(DWORD)(SNDMSG(hwnd, TCM_GETEXTENDEDSTYLE, 0, 0)); }

#undef TabCtrl_SetUnicodeFormat
static FORCEINLINE BOOL TabCtrl_SetUnicodeFormat(_In_ HWND hwnd, _In_ BOOL fUnicode)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwnd, TCM_SETUNICODEFORMAT, STATIC_CAST(WPARAM)(fUnicode), 0)); }

#undef TabCtrl_GetUnicodeFormat
static FORCEINLINE BOOL TabCtrl_GetUnicodeFormat(_In_ HWND hwnd)
	{ return STATIC_CAST(BOOL)(SNDMSG(hwnd, TCM_GETUNICODEFORMAT, 0, 0)); }
#endif /* _WIN32_IE >= 0x0400 */

#endif /* NOTABCONTROL */

//====== ANIMATE CONTROL ======================================================
#ifndef NOANIMATE

#undef Animate_Open
static FORCEINLINE BOOL Animate_Open(_In_ HWND hwnd, _In_ LPCTSTR szName)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, ACM_OPEN, 0, REINTERPRET_CAST(LPARAM)(szName)))); }
#undef Animate_OpenEx
static FORCEINLINE BOOL Animate_OpenEx(_In_ HWND hwnd, _In_ HINSTANCE hInst, _In_ LPCTSTR szName)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, ACM_OPEN, REINTERPRET_CAST(WPARAM)(hInst), REINTERPRET_CAST(LPARAM)(szName)))); }
#undef Animate_Play
static FORCEINLINE BOOL Animate_Play(_In_ HWND hwnd, _In_ UINT from, _In_ UINT to, _In_ UINT rep)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, ACM_PLAY, STATIC_CAST(WPARAM)(rep), STATIC_CAST(LPARAM)(MAKELONG(from, to))))); }
#undef Animate_Stop
static FORCEINLINE BOOL Animate_Stop(_In_ HWND hwnd)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, ACM_STOP, 0, 0))); }
#undef Animate_IsPlaying
static FORCEINLINE BOOL Animate_IsPlaying(_In_ HWND hwnd)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, ACM_ISPLAYING, 0, 0))); }

#endif /* NOANIMATE */

//====== MONTHCAL CONTROL ======================================================
#if (_WIN32_IE >= 0x0300)
#ifndef NOMONTHCAL

#if 0 /* TODO */
#define MonthCal_GetCurSel(hmc, pst)    (BOOL)SNDMSG(hmc, MCM_GETCURSEL, 0, (LPARAM)(pst))

#define MonthCal_SetCurSel(hmc, pst)    (BOOL)SNDMSG(hmc, MCM_SETCURSEL, 0, (LPARAM)(pst))

#define MonthCal_GetMaxSelCount(hmc)    (DWORD)SNDMSG(hmc, MCM_GETMAXSELCOUNT, 0, 0L)

#define MonthCal_SetMaxSelCount(hmc, n) (BOOL)SNDMSG(hmc, MCM_SETMAXSELCOUNT, (WPARAM)(n), 0L)

#define MonthCal_GetSelRange(hmc, rgst) SNDMSG(hmc, MCM_GETSELRANGE, 0, (LPARAM)(rgst))

#define MonthCal_SetSelRange(hmc, rgst) SNDMSG(hmc, MCM_SETSELRANGE, 0, (LPARAM)(rgst))

#define MonthCal_GetMonthRange(hmc, gmr, rgst)  (DWORD)SNDMSG(hmc, MCM_GETMONTHRANGE, (WPARAM)(gmr), (LPARAM)(rgst))

#define MonthCal_SetDayState(hmc, cbds, rgds)   SNDMSG(hmc, MCM_SETDAYSTATE, (WPARAM)(cbds), (LPARAM)(rgds))

#define MonthCal_GetMinReqRect(hmc, prc)        SNDMSG(hmc, MCM_GETMINREQRECT, 0, (LPARAM)(prc))

#define MonthCal_SetColor(hmc, iColor, clr) SNDMSG(hmc, MCM_SETCOLOR, iColor, clr)

#define MonthCal_GetColor(hmc, iColor) SNDMSG(hmc, MCM_GETCOLOR, iColor, 0)

#define MonthCal_SetToday(hmc, pst)             SNDMSG(hmc, MCM_SETTODAY, 0, (LPARAM)(pst))

#define MonthCal_GetToday(hmc, pst)             (BOOL)SNDMSG(hmc, MCM_GETTODAY, 0, (LPARAM)(pst))

#define MonthCal_HitTest(hmc, pinfo) \
        SNDMSG(hmc, MCM_HITTEST, 0, (LPARAM)(PMCHITTESTINFO)(pinfo))

#define MonthCal_SetFirstDayOfWeek(hmc, iDay) \
        SNDMSG(hmc, MCM_SETFIRSTDAYOFWEEK, 0, iDay)

#define MonthCal_GetFirstDayOfWeek(hmc) \
        (DWORD)SNDMSG(hmc, MCM_GETFIRSTDAYOFWEEK, 0, 0)

#define MonthCal_GetRange(hmc, rgst) \
        (DWORD)SNDMSG(hmc, MCM_GETRANGE, 0, (LPARAM)(rgst))

#define MonthCal_SetRange(hmc, gd, rgst) \
        (BOOL)SNDMSG(hmc, MCM_SETRANGE, (WPARAM)(gd), (LPARAM)(rgst))

#define MonthCal_GetMonthDelta(hmc) \
        (int)SNDMSG(hmc, MCM_GETMONTHDELTA, 0, 0)

#define MonthCal_SetMonthDelta(hmc, n) \
        (int)SNDMSG(hmc, MCM_SETMONTHDELTA, n, 0)

#define MonthCal_GetMaxTodayWidth(hmc) \
        (DWORD)SNDMSG(hmc, MCM_GETMAXTODAYWIDTH, 0, 0)

#if (_WIN32_IE >= 0x0400)
#define MonthCal_SetUnicodeFormat(hwnd, fUnicode)  \
    (BOOL)SNDMSG((hwnd), MCM_SETUNICODEFORMAT, (WPARAM)(fUnicode), 0)

#define MonthCal_GetUnicodeFormat(hwnd)  \
    (BOOL)SNDMSG((hwnd), MCM_GETUNICODEFORMAT, 0, 0)
#endif /* _WIN32_IE >= 0x0400 */

#if (NTDDI_VERSION >= NTDDI_VISTA)
#define MonthCal_GetCurrentView(hmc) \
        (DWORD)SNDMSG(hmc, MCM_GETCURRENTVIEW, 0, 0)

#define MonthCal_GetCalendarCount(hmc) \
        (DWORD)SNDMSG(hmc, MCM_GETCALENDARCOUNT, 0, 0)

#define MonthCal_GetCalendarGridInfo(hmc, pmcGridInfo) \
        (BOOL)SNDMSG(hmc, MCM_GETCALENDARGRIDINFO, 0, (LPARAM)(PMCGRIDINFO)(pmcGridInfo))

#define MonthCal_GetCALID(hmc) \
        (CALID)SNDMSG(hmc, MCM_GETCALID, 0, 0)

#define MonthCal_SetCALID(hmc, calid) \
        SNDMSG(hmc, MCM_SETCALID, (WPARAM)(calid), 0)

#define MonthCal_SizeRectToMin(hmc, prc) \
        SNDMSG(hmc, MCM_SIZERECTTOMIN, 0, (LPARAM)(prc))

#define MonthCal_SetCalendarBorder(hmc, fset, xyborder) \
        SNDMSG(hmc, MCM_SETCALENDARBORDER, (WPARAM)(fset), (LPARAM)(xyborder))

#define MonthCal_GetCalendarBorder(hmc) \
        (int)SNDMSG(hmc, MCM_GETCALENDARBORDER, 0, 0)

#define MonthCal_SetCurrentView(hmc, dwNewView) \
        (BOOL)SNDMSG(hmc, MCM_SETCURRENTVIEW, 0, (LPARAM)(dwNewView))
#endif /* NTDDI_VERSION >= NTDDI_VISTA */
#endif /* TODO */

#endif /* NOMONTHCAL */
#endif /* _WIN32_IE >= 0x0300 */

//====== DATETIMEPICK CONTROL ==================================================
#ifndef NODATETIMEPICK

#if 0 /* TODO */
#define DateTime_GetSystemtime(hdp, pst)    (DWORD)SNDMSG(hdp, DTM_GETSYSTEMTIME, 0, (LPARAM)(pst))

#define DateTime_SetSystemtime(hdp, gd, pst)    (BOOL)SNDMSG(hdp, DTM_SETSYSTEMTIME, (WPARAM)(gd), (LPARAM)(pst))

#define DateTime_GetRange(hdp, rgst)  (DWORD)SNDMSG(hdp, DTM_GETRANGE, 0, (LPARAM)(rgst))

#define DateTime_SetRange(hdp, gd, rgst)  (BOOL)SNDMSG(hdp, DTM_SETRANGE, (WPARAM)(gd), (LPARAM)(rgst))

#define DateTime_SetFormat(hdp, sz)  (BOOL)SNDMSG(hdp, DTM_SETFORMAT, 0, (LPARAM)(sz))

#define DateTime_SetMonthCalColor(hdp, iColor, clr) SNDMSG(hdp, DTM_SETMCCOLOR, iColor, clr)

#define DateTime_GetMonthCalColor(hdp, iColor) SNDMSG(hdp, DTM_GETMCCOLOR, iColor, 0)

#define DateTime_GetMonthCal(hdp) (HWND)SNDMSG(hdp, DTM_GETMONTHCAL, 0, 0)

#if (_WIN32_IE >= 0x0400)
#define DateTime_SetMonthCalFont(hdp, hfont, fRedraw) SNDMSG(hdp, DTM_SETMCFONT, (WPARAM)(hfont), (LPARAM)(fRedraw))

#define DateTime_GetMonthCalFont(hdp) SNDMSG(hdp, DTM_GETMCFONT, 0, 0)
#endif /* _WIN32_IE >= 0x0400 */

#if (NTDDI_VERSION >= NTDDI_VISTA)
#define DateTime_SetMonthCalStyle(hdp, dwStyle) SNDMSG(hdp, DTM_SETMCSTYLE, 0, (LPARAM)dwStyle)

#define DateTime_GetMonthCalStyle(hdp) SNDMSG(hdp, DTM_GETMCSTYLE, 0, 0)

#define DateTime_CloseMonthCal(hdp) SNDMSG(hdp, DTM_CLOSEMONTHCAL, 0, 0)

#define DateTime_GetDateTimePickerInfo(hdp, pdtpi) SNDMSG(hdp, DTM_GETDATETIMEPICKERINFO, 0, (LPARAM)(pdtpi))

#define DateTime_GetIdealSize(hdp, psize) (BOOL)SNDMSG((hdp), DTM_GETIDEALSIZE, 0, (LPARAM)(psize))
#endif /* NTDDI_VERSION >= NTDDI_VISTA */
#endif /* TODO */

#endif /* NODATETIMEPICK */

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
//  ====================== Pager Control =============================
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

#ifndef NOPAGESCROLLER

#if 0 /* TODO */
#define Pager_SetChild(hwnd, hwndChild) \
        (void)SNDMSG((hwnd), PGM_SETCHILD, 0, (LPARAM)(hwndChild))

#define Pager_RecalcSize(hwnd) \
        (void)SNDMSG((hwnd), PGM_RECALCSIZE, 0, 0)

#define Pager_ForwardMouse(hwnd, bForward) \
        (void)SNDMSG((hwnd), PGM_FORWARDMOUSE, (WPARAM)(bForward), 0)

#define Pager_SetBkColor(hwnd, clr) \
        (COLORREF)SNDMSG((hwnd), PGM_SETBKCOLOR, 0, (LPARAM)(clr))

#define Pager_GetBkColor(hwnd) \
        (COLORREF)SNDMSG((hwnd), PGM_GETBKCOLOR, 0, 0)

#define Pager_SetBorder(hwnd, iBorder) \
        (int)SNDMSG((hwnd), PGM_SETBORDER, 0, (LPARAM)(iBorder))

#define Pager_GetBorder(hwnd) \
        (int)SNDMSG((hwnd), PGM_GETBORDER, 0, 0)

#define Pager_SetPos(hwnd, iPos) \
        (int)SNDMSG((hwnd), PGM_SETPOS, 0, (LPARAM)(iPos))

#define Pager_GetPos(hwnd) \
        (int)SNDMSG((hwnd), PGM_GETPOS, 0, 0)

#define Pager_SetButtonSize(hwnd, iSize) \
        (int)SNDMSG((hwnd), PGM_SETBUTTONSIZE, 0, (LPARAM)(iSize))

#define Pager_GetButtonSize(hwnd) \
        (int)SNDMSG((hwnd), PGM_GETBUTTONSIZE, 0,0)

#define Pager_GetButtonState(hwnd, iButton) \
        (DWORD)SNDMSG((hwnd), PGM_GETBUTTONSTATE, 0, (LPARAM)(iButton))

#define Pager_GetDropTarget(hwnd, ppdt) \
        (void)SNDMSG((hwnd), PGM_GETDROPTARGET, 0, (LPARAM)(ppdt))
#endif /* TODO */

#endif /* NOPAGESCROLLER */

// ====================== Button Control =============================
#ifndef NOBUTTON

#if (_WIN32_WINNT >= 0x0501)
#undef Button_GetIdealSize
static FORCEINLINE BOOL Button_GetIdealSize(_In_ HWND hwnd, _Out_ SIZE *pSize)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, BCM_GETIDEALSIZE, 0, REINTERPRET_CAST(LPARAM)(pSize)))); }

#undef Button_SetImageList
static FORCEINLINE BOOL Button_SetImageList(_In_ HWND hwnd, _In_ PBUTTON_IMAGELIST pbuttonImageList)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, BCM_SETIMAGELIST, 0, REINTERPRET_CAST(LPARAM)(pbuttonImageList)))); }

#undef Button_SetTextMargin
static FORCEINLINE BOOL Button_SetTextMargin(_In_ HWND hwnd, _In_ const RECT *pmargin)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, BCM_SETTEXTMARGIN, 0, REINTERPRET_CAST(LPARAM)(pmargin)))); }
#undef Button_GetTextMargin
static FORCEINLINE BOOL Button_GetTextMargin(_In_ HWND hwnd, _Out_ RECT *pmargin)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, BCM_GETTEXTMARGIN, 0, REINTERPRET_CAST(LPARAM)(pmargin)))); }
#endif /*_WIN32_WINNT >= 0x0501 */

#if _WIN32_WINNT >= 0x0600
#undef Button_SetDropDownState
static FORCEINLINE BOOL Button_SetDropDownState(_In_ HWND hwnd, _In_ BOOL fDropDown)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, BCM_SETDROPDOWNSTATE, STATIC_CAST(WPARAM)(fDropDown), 0))); }

#undef Button_SetSplitInfo
static FORCEINLINE BOOL Button_SetSplitInfo(_In_ HWND hwnd, _In_ const BUTTON_SPLITINFO *pInfo)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, BCM_SETSPLITINFO, 0, REINTERPRET_CAST(LPARAM)(pInfo)))); }

#undef Button_GetSplitInfo
static FORCEINLINE BOOL Button_GetSplitInfo(_In_ HWND hwnd, _Inout_ BUTTON_SPLITINFO *pInfo)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, BCM_GETSPLITINFO, 0, REINTERPRET_CAST(LPARAM)(pInfo)))); }

#undef Button_SetNote
static FORCEINLINE BOOL Button_SetNote(_In_ HWND hwnd, _In_ PCWSTR psz)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, BCM_SETNOTE, 0, REINTERPRET_CAST(LPARAM)(psz)))); }

#undef Button_GetNote
static FORCEINLINE BOOL Button_GetNote(_In_ HWND hwnd, _Out_ LPWSTR psz, _In_ int pcc)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, BCM_GETNOTE, STATIC_CAST(WPARAM)(pcc), REINTERPRET_CAST(LPARAM)(psz)))); }

#undef Button_GetNoteLength
static FORCEINLINE LRESULT Button_GetNoteLength(_In_ HWND hwnd)
	{ return STATIC_CAST(LRESULT)(SNDMSG(hwnd, BCM_GETNOTELENGTH, 0, 0)); }

#undef Button_SetElevationRequiredState
static FORCEINLINE LRESULT Button_SetElevationRequiredState(_In_ HWND hwnd, _In_ BOOL fRequired)
	{ return STATIC_CAST(LRESULT)(SNDMSG(hwnd, BCM_SETSHIELD, 0, STATIC_CAST(LPARAM)(fRequired))); }
#endif /* _WIN32_WINNT >= 0x0600 */

#endif /* NOBUTTON */

// ====================== Edit Control =============================
#ifndef NOEDIT

#if (_WIN32_WINNT >= 0x0501)
#undef Edit_SetCueBannerText
static FORCEINLINE BOOL Edit_SetCueBannerText(_In_ HWND hwnd, _In_ LPCWSTR lpcwText)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, EM_SETCUEBANNER, 0, REINTERPRET_CAST(LPARAM)(lpcwText)))); }
#undef Edit_SetCueBannerTextFocused
static FORCEINLINE BOOL Edit_SetCueBannerTextFocused(_In_ HWND hwnd, _In_ LPCWSTR lpcwText, _In_ BOOL fDrawFocused)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, EM_SETCUEBANNER, STATIC_CAST(WPARAM)(fDrawFocused), REINTERPRET_CAST(LPARAM)(lpcwText)))); }
#undef Edit_GetCueBannerText
static FORCEINLINE BOOL Edit_GetCueBannerText(_In_ HWND hwnd, _Out_ LPWSTR lpwText, _In_ int cchText)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, EM_GETCUEBANNER, REINTERPRET_CAST(WPARAM)(lpwText), STATIC_CAST(LPARAM)(cchText)))); }

#undef Edit_ShowBalloonTip
static FORCEINLINE BOOL Edit_ShowBalloonTip(_In_ HWND hwnd, _In_ PEDITBALLOONTIP peditballoontip)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, EM_SHOWBALLOONTIP, 0, REINTERPRET_CAST(LPARAM)(peditballoontip)))); }
#undef Edit_HideBalloonTip
static FORCEINLINE BOOL Edit_HideBalloonTip(_In_ HWND hwnd)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, EM_SHOWBALLOONTIP, 0, 0))); }
#endif /* _WIN32_WINNT >= 0x0501 */

#if _WIN32_WINNT >= 0x0600
// NOTE: Not actually used for anything.
// Reference: https://devblogs.microsoft.com/oldnewthing/20071025-00/?p=24693
#undef Edit_SetHilite
static FORCEINLINE void Edit_SetHilite(_In_ HWND hwndCtl, _In_ int ichStart, _In_ int ichEnd)
	{ (void)SNDMSG(hwndCtl, EM_SETHILITE, STATIC_CAST(WPARAM)(ichStart), STATIC_CAST(LPARAM)(ichEnd)); }
#undef Edit_GetHilite
static FORCEINLINE DWORD Edit_GetHilite(_In_ HWND hwndCtl)
	{ return STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_GETHILITE, 0L, 0L)); }
#endif /* _WIN32_WINNT */

#endif /* NOEDIT */

// ====================== Combobox Control =============================
#ifndef NOCOMBOBOX
#if (_WIN32_WINNT >= 0x0501)
#undef ComboBox_SetMinVisible
static FORCEINLINE BOOL ComboBox_SetMinVisible(_In_ HWND hwnd, _In_ int iMinVisible)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, CB_SETMINVISIBLE, STATIC_CAST(WPARAM)(iMinVisible), 0))); }

#undef ComboBox_GetMinVisible
static FORCEINLINE int ComboBox_GetMinVisible(_In_ HWND hwnd)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwnd, CB_GETMINVISIBLE, 0, 0))); }

#undef ComboBox_SetCueBannerText
static FORCEINLINE BOOL ComboBox_SetCueBannerText(_In_ HWND hwnd, _In_ LPCWSTR lpcwText)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, CB_SETCUEBANNER, 0, REINTERPRET_CAST(LPARAM)(lpcwText)))); }

#undef ComboBox_GetCueBannerText
static FORCEINLINE BOOL ComboBox_GetCueBannerText(_In_ HWND hwnd, _Out_ LPWSTR lpwText, _In_ int cchText)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwnd, CB_SETCUEBANNER, REINTERPRET_CAST(WPARAM)(lpwText), STATIC_CAST(LPARAM)(cchText)))); }
#endif /* _WIN32_WINNT >= 0x0501 */
#endif /* NOCOMBOBOX */
