/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * windowsx_ts.h: Typesafe inline function wrappers for windowsx.h.        *
 *                                                                         *
 * Based on windowsx.h from the Windows SDK v7.1A.                         *
 ***************************************************************************/

#pragma once

#include "tsbase.h"
#include <windowsx.h>

// C includes.
#include <assert.h>

// NOTE: Afx*() functions (_MAC) are not included here.

/****** GDI Macro APIs *******************************************************/

#undef DeletePen
static FORCEINLINE BOOL DeletePen(_In_ HPEN hPen)
	{ return DeleteObject(STATIC_CAST(HGDIOBJ)(hPen)); }
#undef SelectPen
static FORCEINLINE HPEN SelectPen(_In_ HDC hDC, _In_ HPEN hPen)
	{ return STATIC_CAST(HPEN)(SelectObject(hDC, STATIC_CAST(HGDIOBJ)(hPen))); }
#undef GetStockPen
static FORCEINLINE HPEN GetStockPen(_In_ int fnPen)
	{ return STATIC_CAST(HPEN)(GetStockObject(fnPen)); }

#undef DeleteBrush
static FORCEINLINE BOOL DeleteBrush(_In_ HBRUSH hBrush)
	{ return DeleteObject(STATIC_CAST(HGDIOBJ)(hBrush)); }
#undef SelectBrush
static FORCEINLINE HBRUSH SelectBrush(_In_ HDC hDC, _In_ HBRUSH hBrush)
	{ return STATIC_CAST(HBRUSH)(SelectObject(hDC, STATIC_CAST(HGDIOBJ)(hBrush))); }
#undef GetStockBrush
static FORCEINLINE HBRUSH GetStockBrush(_In_ int fnBrush)
	{ return STATIC_CAST(HBRUSH)(GetStockObject(fnBrush)); }

#undef DeletePalette
static FORCEINLINE BOOL DeletePalette(_In_ HPALETTE hPal)
	{ return DeleteObject(STATIC_CAST(HGDIOBJ)(hPal)); }

#undef DeleteFont
static FORCEINLINE BOOL DeleteFont(_In_ HFONT hFont)
	{ return DeleteObject(STATIC_CAST(HGDIOBJ)(hFont)); }
#undef SelectFont
static FORCEINLINE HFONT SelectFont(_In_ HDC hDC, _In_ HFONT hFont)
	{ return STATIC_CAST(HFONT)(SelectObject(hDC, STATIC_CAST(HGDIOBJ)(hFont))); }
#undef GetStockFont
static FORCEINLINE HFONT GetStockFont(_In_ int fnFont)
	{ return STATIC_CAST(HFONT)(GetStockObject(fnFont)); }

#undef DeleteBitmap
static FORCEINLINE BOOL DeleteBitmap(_In_ HBITMAP hbm)
	{ return DeleteObject(STATIC_CAST(HGDIOBJ)(hbm)); }
#undef SelectBitmap
static FORCEINLINE HBITMAP SelectBitmap(_In_ HDC hDC, _In_ HBITMAP hbm)
	{ return STATIC_CAST(HBITMAP)(SelectObject(hDC, STATIC_CAST(HGDIOBJ)(hbm))); }

/****** USER Macro APIs ******************************************************/

#undef SetWindowRedraw
static FORCEINLINE void SetWindowRedraw(_In_ HWND hWnd, _In_ BOOL fRedraw)
	{ (void)SNDMSG(hWnd, WM_SETREDRAW, STATIC_CAST(WPARAM)(fRedraw), 0L); }

#undef SubclassWindow
static FORCEINLINE WNDPROC SubclassWindow(_In_ HWND hWnd, _In_ WNDPROC lpFn)
	{ return REINTERPRET_CAST(WNDPROC)(SetWindowLongPtr(hWnd, GWLP_WNDPROC, REINTERPRET_CAST(LPARAM)(lpFn))); }

#undef SetWindowFont
static FORCEINLINE void SetWindowFont(_In_ HWND hWnd, _In_ HFONT hFont, _In_ BOOL fRedraw)
	{ (void)SNDMSG(hWnd, WM_SETFONT, REINTERPRET_CAST(WPARAM)(hFont), STATIC_CAST(LPARAM)(fRedraw)); }

#undef GetWindowFont
static FORCEINLINE HFONT GetWindowFont(_In_ HWND hWnd)
	{ return REINTERPRET_CAST(HFONT)(SNDMSG(hWnd, WM_GETFONT, 0L, 0L)); }

#undef MapWindowRect
static FORCEINLINE int MapWindowRect(_In_ HWND hwndFrom, _In_ HWND hwndTo, _Inout_ LPRECT lprc)
	{ return MapWindowPoints(hwndFrom, hwndTo, REINTERPRET_CAST(POINT*)(lprc), 2); }

#undef SubclassDialog
static FORCEINLINE DLGPROC SubclassDialog(_In_ HWND hwndDlg, _In_ DLGPROC lpFn)
	{ return REINTERPRET_CAST(DLGPROC)(SetWindowLongPtr(hwndDlg, DWLP_DLGPROC, REINTERPRET_CAST(LPARAM)(lpFn))); }

/****** Message crackers ****************************************************/

/** These macros have been skipped because they aren't very useful nowadays. **/

/****** Control message APIs *************************************************/

/**
 * NOTE: Standard *_Enable(), *_GetText(), *_GetTextLength(), *_SetText(), and
 * *_Show() macros have been skipped, since the parameters map directly to
 * function arguments instead of being casted to SendMessage() parameters.
 */

/****** Static control message APIs ******************************************/

#undef Static_SetIcon
static FORCEINLINE HICON Static_SetIcon(_In_ HWND hwndCtl, _In_ HICON hIcon)
	{ return REINTERPRET_CAST(HICON)(STATIC_CAST(UINT_PTR)(SNDMSG(hwndCtl, STM_SETICON, REINTERPRET_CAST(WPARAM)(hIcon), 0L))); }
#undef Static_GetIcon
static FORCEINLINE HICON Static_GetIcon(_In_ HWND hwndCtl)
	{ return REINTERPRET_CAST(HICON)(STATIC_CAST(UINT_PTR)(SNDMSG(hwndCtl, STM_GETICON, 0L, 0L))); }


/****** Button control message APIs ******************************************/

#undef Button_GetCheck
static FORCEINLINE int Button_GetCheck(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, BM_GETCHECK, 0L, 0L))); }
#undef Button_SetCheck
static FORCEINLINE void Button_SetCheck(_In_ HWND hwndCtl, _In_ int check)
	{ (void)SNDMSG(hwndCtl, BM_SETCHECK, STATIC_CAST(WPARAM)(check), 0L); }

#undef Button_GetState
static FORCEINLINE int Button_GetState(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, BM_GETSTATE, 0L, 0L))); }
#undef Button_SetState
static FORCEINLINE UINT Button_SetState(_In_ HWND hwndCtl, _In_ int state)
	{ return STATIC_CAST(UINT)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, BM_SETSTATE, STATIC_CAST(WPARAM)(state), 0L))); }

#undef Button_SetStyle
static FORCEINLINE void Button_SetStyle(_In_ HWND hwndCtl, _In_ DWORD style, _In_ BOOL fRedraw)
	{ (void)SNDMSG(hwndCtl, BM_SETSTYLE, STATIC_CAST(WPARAM)(LOWORD(style)), MAKELPARAM(!!fRedraw, 0)); }

/****** Edit control message APIs ********************************************/

#undef Edit_LimitText
static FORCEINLINE void Edit_LimitText(_In_ HWND hwndCtl, _In_ int cchMax)
	{ (void)SNDMSG(hwndCtl, EM_LIMITTEXT, STATIC_CAST(WPARAM)(cchMax), 0L); }

#undef Edit_GetLineCount
static FORCEINLINE DWORD Edit_GetLineCount(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_GETLINECOUNT, 0L, 0L))); }
#undef Edit_GetLine
static FORCEINLINE int Edit_GetLine(_In_ HWND hwndCtl, _In_ int line, _Inout_ LPTSTR lpch, _In_ int cchMax)
{
	// cchMax must be copied into the buffer.
	assert((sizeof(TCHAR) == 1 && cchMax >= 4) || (sizeof(TCHAR) == 2 && cchMax >= 2));
	(*(int*)lpch) = cchMax;
	return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_GETLINE, STATIC_CAST(WPARAM)(line), REINTERPRET_CAST(LPARAM)(lpch))));
}

#undef Edit_GetRect
static FORCEINLINE void Edit_GetRect(_In_ HWND hwndCtl, _Out_ RECT *lprc)
	{ (void)SNDMSG(hwndCtl, EM_GETRECT, 0L, REINTERPRET_CAST(LPARAM)(lprc)); }
#undef Edit_SetRect
static FORCEINLINE void Edit_SetRect(_In_ HWND hwndCtl, _In_ const RECT *lprc)
	{ (void)SNDMSG(hwndCtl, EM_SETRECT, 0L, REINTERPRET_CAST(LPARAM)(lprc)); }
#undef Edit_SetRectNoPaint
static FORCEINLINE void Edit_SetRectNoPaint(_In_ HWND hwndCtl, _In_ const RECT *lprc)
	{ (void)SNDMSG(hwndCtl, EM_SETRECTNP, 0L, REINTERPRET_CAST(LPARAM)(lprc)); }

#undef Edit_GetSel
static FORCEINLINE DWORD Edit_GetSel(_In_ HWND hwndCtl)
	{ return STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_GETSEL, 0L, 0L)); }
#undef Edit_SetSel
static FORCEINLINE void Edit_SetSel(_In_ HWND hwndCtl, _In_ int ichStart, _In_ int ichEnd)
	{ (void)SNDMSG(hwndCtl, EM_SETSEL, STATIC_CAST(WPARAM)(ichStart), STATIC_CAST(LPARAM)(ichEnd)); }
#undef Edit_ReplaceSel
static FORCEINLINE void Edit_ReplaceSel(_In_ HWND hwndCtl, _In_ LPCTSTR lpszReplace)
	{ (void)SNDMSG(hwndCtl, EM_REPLACESEL, 0L, REINTERPRET_CAST(LPARAM)(lpszReplace)); }

#undef Edit_GetModify
static FORCEINLINE BOOL Edit_GetModify(_In_ HWND hwndCtl)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_GETMODIFY, 0L, 0L))); }
#undef Edit_SetModify
static FORCEINLINE void Edit_SetModify(_In_ HWND hwndCtl, _In_ BOOL fModified)
	{ (void)SNDMSG(hwndCtl, EM_SETMODIFY, STATIC_CAST(WPARAM)(STATIC_CAST(UINT)(fModified)), 0L); }

#undef Edit_ScrollCaret
static FORCEINLINE BOOL Edit_ScrollCaret(_In_ HWND hwndCtl)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_SCROLLCARET, 0, 0L))); }

#undef Edit_LineFromChar
static FORCEINLINE int Edit_LineFromChar(_In_ HWND hwndCtl, _In_ int ich)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_LINEFROMCHAR, STATIC_CAST(WPARAM)(ich), 0L))); }
#undef Edit_LineIndex
static FORCEINLINE int Edit_LineIndex(_In_ HWND hwndCtl, _In_ int line)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_LINEINDEX, STATIC_CAST(WPARAM)(line), 0L))); }
#undef Edit_LineLength
static FORCEINLINE int Edit_LineLength(_In_ HWND hwndCtl, _In_ int line)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_LINELENGTH, STATIC_CAST(WPARAM)(line), 0L))); }

#undef Edit_Scroll
static FORCEINLINE void Edit_Scroll(_In_ HWND hwndCtl, _In_ int dv, _In_ int dh)
	{ (void)SNDMSG(hwndCtl, EM_LINESCROLL, STATIC_CAST(WPARAM)(dh), STATIC_CAST(LPARAM)(dv)); }

#undef Edit_CanUndo
static FORCEINLINE BOOL Edit_CanUndo(_In_ HWND hwndCtl)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_CANUNDO, 0L, 0L))); }
#undef Edit_Undo
static FORCEINLINE BOOL Edit_Undo(_In_ HWND hwndCtl)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_UNDO, 0L, 0L))); }
#undef Edit_EmptyUndoBuffer
static FORCEINLINE void Edit_EmptyUndoBuffer(_In_ HWND hwndCtl)
	{ (void)SNDMSG(hwndCtl, EM_EMPTYUNDOBUFFER, 0L, 0L); }

#undef Edit_SetPasswordChar
static FORCEINLINE void Edit_SetPasswordChar(_In_ HWND hwndCtl, _In_ TCHAR ch)
	{ (void)SNDMSG(hwndCtl, EM_SETPASSWORDCHAR, STATIC_CAST(WPARAM)(STATIC_CAST(UINT)(ch)), 0L); }

#undef Edit_SetTabStops
static FORCEINLINE void Edit_SetTabStops(_In_ HWND hwndCtl, _In_ int cTabs, _In_ const int *lpTabs)
	{ (void)SNDMSG(hwndCtl, EM_SETTABSTOPS, STATIC_CAST(WPARAM)(cTabs), REINTERPRET_CAST(LPARAM)(lpTabs)); }

#undef Edit_FmtLines
static FORCEINLINE BOOL Edit_FmtLines(_In_ HWND hwndCtl, _In_ BOOL fAddEOL)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_FMTLINES, STATIC_CAST(WPARAM)(fAddEOL), 0L))); }

#undef Edit_GetHandle
static FORCEINLINE HLOCAL Edit_GetHandle(_In_ HWND hwndCtl)
	{ return REINTERPRET_CAST(HLOCAL)(STATIC_CAST(UINT_PTR)(SNDMSG(hwndCtl, EM_GETHANDLE, 0L, 0L))); }
#undef Edit_SetHandle
static FORCEINLINE void Edit_SetHandle(_In_ HWND hwndCtl, _In_ HLOCAL h)
	{ (void)SNDMSG(hwndCtl, EM_SETHANDLE, STATIC_CAST(WPARAM)(REINTERPRET_CAST(UINT_PTR)(h)), 0L); }

#if (WINVER >= 0x030a)
#undef Edit_GetFirstVisibleLine
static FORCEINLINE int Edit_GetFirstVisibleLine(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_GETFIRSTVISIBLELINE, 0L, 0L))); }

#undef Edit_SetReadOnly
static FORCEINLINE BOOL Edit_SetReadOnly(_In_ HWND hwndCtl, _In_ BOOL fReadOnly)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_SETREADONLY, STATIC_CAST(WPARAM)(fReadOnly), 0L))); }

#undef Edit_GetPasswordChar
static FORCEINLINE TCHAR Edit_GetPasswordChar(_In_ HWND hwndCtl)
	{ return STATIC_CAST(TCHAR)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, EM_GETPASSWORDCHAR, 0L, 0L))); }

#undef Edit_SetWordBreakProc
static FORCEINLINE void Edit_SetWordBreakProc(_In_ HWND hwndCtl, _In_ EDITWORDBREAKPROC lpfnWordBreak)
	{ (void)SNDMSG(hwndCtl, EM_SETWORDBREAKPROC, 0L, REINTERPRET_CAST(LPARAM)(lpfnWordBreak)); }
#undef Edit_GetWordBreakProc
static FORCEINLINE EDITWORDBREAKPROC Edit_GetWordBreakProc(_In_ HWND hwndCtl)
	{ return REINTERPRET_CAST(EDITWORDBREAKPROC)(SNDMSG(hwndCtl, EM_GETWORDBREAKPROC, 0L, 0L)); }
#endif /* WINVER >= 0x030a */

/****** ScrollBar control message APIs ***************************************/

/* All ScrollBar control message APIs are typesafe, *
 * since they use scroll-specific functions.        */

/****** ListBox control message APIs *****************************************/

#undef ListBox_GetCount
static FORCEINLINE int ListBox_GetCount(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_GETCOUNT, 0L, 0L))); }
#undef ListBox_ResetContent
static FORCEINLINE int ListBox_ResetContent(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_RESETCONTENT, 0L, 0L))); }

#undef ListBox_AddString
static FORCEINLINE int ListBox_AddString(_In_ HWND hwndCtl, _In_ LPCTSTR lpsz)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_ADDSTRING, 0L, REINTERPRET_CAST(LPARAM)(lpsz)))); }
#undef ListBox_InsertString
static FORCEINLINE int ListBox_InsertString(_In_ HWND hwndCtl, _In_ int index, _In_ LPCTSTR lpsz)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_INSERTSTRING, STATIC_CAST(WPARAM)(index), REINTERPRET_CAST(LPARAM)(lpsz)))); }

#undef ListBox_AddItemData
static FORCEINLINE int ListBox_AddItemData(_In_ HWND hwndCtl, _In_ LPARAM data)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_ADDSTRING, 0L, data))); }
#undef ListBox_InsertItemData
static FORCEINLINE int ListBox_InsertItemData(_In_ HWND hwndCtl, _In_ int index, _In_ LPARAM data)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_INSERTSTRING, STATIC_CAST(WPARAM)(index), data))); }

#undef ListBox_DeleteString
static FORCEINLINE int ListBox_DeleteString(_In_ HWND hwndCtl, _In_ int index)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_DELETESTRING, STATIC_CAST(WPARAM)(index), 0L))); }

#undef ListBox_GetTextLen
static FORCEINLINE int ListBox_GetTextLen(_In_ HWND hwndCtl, _In_ int index)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_GETTEXTLEN, STATIC_CAST(WPARAM)(index), 0L))); }
#undef ListBox_GetText
static FORCEINLINE int ListBox_GetText(_In_ HWND hwndCtl, _In_ int index, _Out_ LPTSTR lpszBuffer)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_GETTEXT, STATIC_CAST(WPARAM)(index), REINTERPRET_CAST(LPARAM)(lpszBuffer)))); }

#undef ListBox_GetItemData
static FORCEINLINE LRESULT ListBox_GetItemData(_In_ HWND hwndCtl, _In_ int index)
	{ return STATIC_CAST(LRESULT)(STATIC_CAST(ULONG_PTR)(SNDMSG(hwndCtl, LB_GETITEMDATA, STATIC_CAST(WPARAM)(index), 0L))); }
#undef ListBox_SetItemData
static FORCEINLINE int ListBox_SetItemData(_In_ HWND hwndCtl, _In_ int index, _In_ LPARAM data)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_SETITEMDATA, STATIC_CAST(WPARAM)(index), data))); }

#if (WINVER >= 0x030a)
#undef ListBox_FindString
static FORCEINLINE int ListBox_FindString(_In_ HWND hwndCtl, _In_ int indexStart, _In_ LPCTSTR lpszFind)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_FINDSTRING, STATIC_CAST(WPARAM)(indexStart), REINTERPRET_CAST(LPARAM)(lpszFind)))); }
#undef ListBox_FindItemData
static FORCEINLINE int ListBox_FindItemData(_In_ HWND hwndCtl, _In_ int indexStart, _In_ LPARAM data)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_FINDSTRING, STATIC_CAST(WPARAM)(indexStart), data))); }

#undef ListBox_SetSel
static FORCEINLINE int ListBox_SetSel(_In_ HWND hwndCtl, _In_ BOOL fSelect, _In_ int index)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_SETSEL, STATIC_CAST(WPARAM)(fSelect), STATIC_CAST(LPARAM)(index)))); }
#undef ListBox_SelItemRange
static FORCEINLINE int ListBox_SelItemRange(_In_ HWND hwndCtl, _In_ BOOL fSelect, _In_ int first, _In_ int last)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_SELITEMRANGE, STATIC_CAST(WPARAM)(fSelect), MAKELPARAM(first, last)))); }

#undef ListBox_GetCurSel
static FORCEINLINE int ListBox_GetCurSel(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_GETCURSEL, 0L, 0L))); }
#undef ListBox_SetCurSel
static FORCEINLINE int ListBox_SetCurSel(_In_ HWND hwndCtl, _In_ int index)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_SETCURSEL, STATIC_CAST(WPARAM)(index), 0L))); }

#undef ListBox_SelectString
static FORCEINLINE int ListBox_SelectString(_In_ HWND hwndCtl, _In_ int indexStart, _In_ LPCTSTR lpszFind)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_SELECTSTRING, STATIC_CAST(WPARAM)(indexStart), REINTERPRET_CAST(LPARAM)(lpszFind)))); }
#undef ListBox_SelectItemData
static FORCEINLINE int ListBox_SelectItemData(_In_ HWND hwndCtl, _In_ int indexStart, _In_ LPARAM data)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_SELECTSTRING, STATIC_CAST(WPARAM)(indexStart), data))); }

#undef ListBox_GetSel
static FORCEINLINE int ListBox_GetSel(_In_ HWND hwndCtl, _In_ int index)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_GETSEL, STATIC_CAST(WPARAM)(index), 0L))); }
#undef ListBox_GetSelCount
static FORCEINLINE int ListBox_GetSelCount(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_GETSELCOUNT, 0L, 0L))); }
#undef ListBox_GetTopIndex
static FORCEINLINE int ListBox_GetTopIndex(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_GETTOPINDEX, 0L, 0L))); }
#undef ListBox_GetSelItems
static FORCEINLINE int ListBox_GetSelItems(_In_ HWND hwndCtl, _In_ int cItems, _Out_ int *lpItems)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_GETSELITEMS, STATIC_CAST(WPARAM)(cItems), REINTERPRET_CAST(LPARAM)(lpItems)))); }

#undef ListBox_SetTopIndex
static FORCEINLINE int ListBox_SetTopIndex(_In_ HWND hwndCtl, _In_ int indexTop)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_SETTOPINDEX, STATIC_CAST(WPARAM)(indexTop), 0L))); }

#undef ListBox_SetColumnWidth
static FORCEINLINE void ListBox_SetColumnWidth(_In_ HWND hwndCtl, _In_ int cxColumn)
	{ (void)SNDMSG(hwndCtl, LB_SETCOLUMNWIDTH, STATIC_CAST(WPARAM)(cxColumn), 0L); }
#undef ListBox_GetHorizontalExtent
static FORCEINLINE int ListBox_GetHorizontalExtent(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_GETHORIZONTALEXTENT, 0L, 0L))); }
#undef ListBox_SetHorizontalExtent
static FORCEINLINE void ListBox_SetHorizontalExtent(_In_ HWND hwndCtl, _In_ int cxExtent)
	{ (void)SNDMSG(hwndCtl, LB_SETHORIZONTALEXTENT, STATIC_CAST(WPARAM)(cxExtent), 0L); }

#undef ListBox_SetTabStops
static FORCEINLINE BOOL ListBox_SetTabStops(_In_ HWND hwndCtl, _In_ int cTabs, _In_ const int *lpTabs)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_SETTABSTOPS, STATIC_CAST(WPARAM)(cTabs), REINTERPRET_CAST(LPARAM)(lpTabs)))); }

#undef ListBox_GetItemRect
static FORCEINLINE int ListBox_GetItemRect(_In_ HWND hwndCtl, _In_ int index, _Out_ RECT *lprc)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_GETITEMRECT, STATIC_CAST(WPARAM)(index), REINTERPRET_CAST(LPARAM)(lprc)))); }

#undef ListBox_SetCaretIndex
static FORCEINLINE int ListBox_SetCaretIndex(_In_ HWND hwndCtl, _In_ int index)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_SETCARETINDEX, STATIC_CAST(WPARAM)(index), 0L))); }
#undef ListBox_GetCaretIndex
static FORCEINLINE int ListBox_GetCaretIndex(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_GETCARETINDEX, 0L, 0L))); }

#undef ListBox_FindStringExact
static FORCEINLINE int ListBox_FindStringExact(_In_ HWND hwndCtl, _In_ int indexStart, _In_ LPCTSTR lpszFind)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_FINDSTRINGEXACT, STATIC_CAST(WPARAM)(indexStart), REINTERPRET_CAST(LPARAM)(lpszFind)))); }

#undef ListBox_SetItemHeight
static FORCEINLINE int ListBox_SetItemHeight(_In_ HWND hwndCtl, _In_ int index, _In_ int cy)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_SETITEMHEIGHT, STATIC_CAST(WPARAM)(index), MAKELPARAM(cy, 0)))); }
#undef ListBox_GetItemHeight
static FORCEINLINE int ListBox_GetItemHeight(_In_ HWND hwndCtl, _In_ int index)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_GETITEMHEIGHT, STATIC_CAST(WPARAM)(index), 0L))); }
#endif  /* WINVER >= 0x030a */

#undef ListBox_Dir
static FORCEINLINE int ListBox_Dir(_In_ HWND hwndCtl, _In_ UINT attrs, _In_ LPCTSTR lpszFileSpec)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, LB_DIR, STATIC_CAST(WPARAM)(attrs), REINTERPRET_CAST(LPARAM)(lpszFileSpec)))); }

/****** ComboBox control message APIs ****************************************/

#undef ComboBox_LimitText
static FORCEINLINE int ComboBox_LimitText(_In_ HWND hwndCtl, _In_ int cchLimit)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_LIMITTEXT, STATIC_CAST(WPARAM)(cchLimit), 0L))); }

#undef ComboBox_GetEditSel
static FORCEINLINE DWORD ComboBox_GetEditSel(_In_ HWND hwndCtl)
	{ return STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_GETEDITSEL, 0L, 0L)); }
#undef ComboBox_SetEditSel
static FORCEINLINE int ComboBox_SetEditSel(_In_ HWND hwndCtl, _In_ int ichStart, _In_ int ichEnd)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_SETEDITSEL, 0L, MAKELPARAM(ichStart, ichEnd)))); }

#undef ComboBox_GetCount
static FORCEINLINE int ComboBox_GetCount(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_GETCOUNT, 0L, 0L))); }
#undef ComboBox_ResetContent
static FORCEINLINE int ComboBox_ResetContent(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_RESETCONTENT, 0L, 0L))); }

#undef ComboBox_AddString
static FORCEINLINE int ComboBox_AddString(_In_ HWND hwndCtl, _In_ LPCTSTR lpsz)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_ADDSTRING, 0L, REINTERPRET_CAST(LPARAM)(lpsz)))); }
#undef ComboBox_InsertString
static FORCEINLINE int ComboBox_InsertString(_In_ HWND hwndCtl, _In_ int index, _In_ LPCTSTR lpsz)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_INSERTSTRING, STATIC_CAST(WPARAM)(index), REINTERPRET_CAST(LPARAM)(lpsz)))); }

#undef ComboBox_AddItemData
static FORCEINLINE int ComboBox_AddItemData(_In_ HWND hwndCtl, _In_ LPARAM data)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_ADDSTRING, 0L, data))); }
#undef ComboBox_InsertItemData
static FORCEINLINE int ComboBox_InsertItemData(_In_ HWND hwndCtl, _In_ int index, _In_ LPARAM data)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_INSERTSTRING, STATIC_CAST(WPARAM)(index), data))); }

#undef ComboBox_DeleteString
static FORCEINLINE int ComboBox_DeleteString(_In_ HWND hwndCtl, _In_ int index)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_DELETESTRING, STATIC_CAST(WPARAM)(index), 0L))); }

#undef ComboBox_GetLBTextLen
static FORCEINLINE int ComboBox_GetLBTextLen(_In_ HWND hwndCtl, _In_ int index)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_GETLBTEXTLEN, STATIC_CAST(WPARAM)(index), 0L))); }
#undef ComboBox_GetLBText
static FORCEINLINE int ComboBox_GetLBText(_In_ HWND hwndCtl, _In_ int index, _Out_ LPTSTR lpszBuffer)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_GETLBTEXT, STATIC_CAST(WPARAM)(index), REINTERPRET_CAST(LPARAM)(lpszBuffer)))); }

#undef ComboBox_GetItemData
static FORCEINLINE LRESULT ComboBox_GetItemData(_In_ HWND hwndCtl, _In_ int index)
	{ return STATIC_CAST(LRESULT)(STATIC_CAST(ULONG_PTR)(SNDMSG(hwndCtl, CB_GETITEMDATA, STATIC_CAST(WPARAM)(index), 0L))); }
#undef ComboBox_SetItemData
static FORCEINLINE int ComboBox_SetItemData(_In_ HWND hwndCtl, _In_ int index, _In_ LPARAM data)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_SETITEMDATA, STATIC_CAST(WPARAM)(index), data))); }

#undef ComboBox_FindString
static FORCEINLINE int ComboBox_FindString(_In_ HWND hwndCtl, _In_ int indexStart, _In_ LPCTSTR lpszFind)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_FINDSTRING, STATIC_CAST(WPARAM)(indexStart), REINTERPRET_CAST(LPARAM)(lpszFind)))); }
#undef ComboBox_FindItemData
static FORCEINLINE int ComboBox_FindItemData(_In_ HWND hwndCtl, _In_ int indexStart, _In_ LPARAM data)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_FINDSTRING, STATIC_CAST(WPARAM)(indexStart), data))); }

#undef ComboBox_GetCurSel
static FORCEINLINE int ComboBox_GetCurSel(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_GETCURSEL, 0L, 0L))); }
#undef ComboBox_SetCurSel
static FORCEINLINE int ComboBox_SetCurSel(_In_ HWND hwndCtl, _In_ int index)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_SETCURSEL, STATIC_CAST(WPARAM)(index), 0L))); }

#undef ComboBox_SelectString
static FORCEINLINE int ComboBox_SelectString(_In_ HWND hwndCtl, _In_ int indexStart, _In_ LPCTSTR lpszSelect)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_SELECTSTRING, STATIC_CAST(WPARAM)(indexStart), REINTERPRET_CAST(LPARAM)(lpszSelect)))); }
#undef ComboBox_SelectItemData
static FORCEINLINE int ComboBox_SelectItemData(_In_ HWND hwndCtl, _In_ int indexStart, _In_ LPARAM data)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_SELECTSTRING, STATIC_CAST(WPARAM)(indexStart), data))); }

#undef ComboBox_Dir
static FORCEINLINE int ComboBox_Dir(_In_ HWND hwndCtl, _In_ UINT attrs, _In_ LPCTSTR lpszFileSpec)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_DIR, STATIC_CAST(WPARAM)(attrs), REINTERPRET_CAST(LPARAM)(lpszFileSpec)))); }

#undef ComboBox_ShowDropdown
static FORCEINLINE BOOL ComboBox_ShowDropdown(_In_ HWND hwndCtl, _In_ BOOL fShow)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_SHOWDROPDOWN, STATIC_CAST(WPARAM)(fShow), 0L))); }

#if (WINVER >= 0x030a)
#undef ComboBox_FindStringExact
static FORCEINLINE int ComboBox_FindStringExact(_In_ HWND hwndCtl, _In_ int indexStart, _In_ LPCTSTR lpszFind)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_FINDSTRINGEXACT, STATIC_CAST(WPARAM)(indexStart), REINTERPRET_CAST(LPARAM)(lpszFind)))); }

#undef ComboBox_GetDroppedState
static FORCEINLINE BOOL ComboBox_GetDroppedState(_In_ HWND hwndCtl)
	{ return STATIC_CAST(BOOL)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_GETDROPPEDSTATE, 0L, 0L))); }
#undef ComboBox_GetDroppedControlRect
static FORCEINLINE void ComboBox_GetDroppedControlRect(_In_ HWND hwndCtl, _Out_ RECT *lprc)
	{ (void)SNDMSG(hwndCtl, CB_GETDROPPEDCONTROLRECT, 0L, REINTERPRET_CAST(LPARAM)(lprc)); }

#undef ComboBox_GetItemHeight
static FORCEINLINE int ComboBox_GetItemHeight(_In_ HWND hwndCtl)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_GETITEMHEIGHT, 0L, 0L))); }
#undef ComboBox_SetItemHeight
static FORCEINLINE int ComboBox_SetItemHeight(_In_ HWND hwndCtl, _In_ int index, _In_ int cyItem)
	{ return STATIC_CAST(int)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_SETITEMHEIGHT, STATIC_CAST(WPARAM)(index), STATIC_CAST(LPARAM)(cyItem)))); }

#undef ComboBox_GetExtendedUI
static FORCEINLINE UINT ComboBox_GetExtendedUI(_In_ HWND hwndCtl)
	{ return STATIC_CAST(UINT)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_GETEXTENDEDUI, 0L, 0L))); }
#undef ComboBox_SetExtendedUI
static FORCEINLINE int ComboBox_SetExtendedUI(_In_ HWND hwndCtl, _In_ UINT flags)
	{ return STATIC_CAST(UINT)(STATIC_CAST(DWORD)(SNDMSG(hwndCtl, CB_SETEXTENDEDUI, STATIC_CAST(WPARAM)(flags), 0L))); }
#endif  /* WINVER >= 0x030a */
