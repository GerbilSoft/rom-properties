/**
 * Custom subclasses for full Dark Mode functionality.
 *
 * Based on Notepad++'s controls:
 * https://github.com/notepad-plus-plus/notepad-plus-plus/tree/master/PowerEditor/src/WinControls
 */

#include "NppDarkMode.hpp"
#include "DarkMode.hpp"
#include "DarkModeCtrl.hpp"	// for dark mode colors

// C includes (for _countof())
#include <stdlib.h>

// Extra Win32 includes
#include "libwin32common/sdk/windowsx_ts.h"
#include <tchar.h>

// High DPI support
#include "libwin32ui/HiDPI.h"

static const COLORREF colorEdge = RGB(100,100,100);
static HPEN hpenEdge = nullptr;
static HBRUSH hbrEdge = nullptr;

/**
 * WM_DRAWITEM handler for TabControl.
 * @param hWnd TabControl
 * @param pDrawItemStruct DRAWITEMSTRUCT
 */
static void NppDarkMode_TabControl_drawItem(HWND hWnd, const DRAWITEMSTRUCT *pDrawItemStruct)
{
	RECT rect = pDrawItemStruct->rcItem;

	const int nTab = pDrawItemStruct->itemID;
	assert(nTab >= 0);
	const bool isSelected = (nTab == TabCtrl_GetCurSel(hWnd));

	TCHAR label[MAX_PATH];
	label[0] = _T('\0');
	TCITEM tci;
	tci.mask = TCIF_TEXT;
	tci.pszText = label;
	tci.cchTextMax = _countof(label) - 1;

	BOOL bRet = TabCtrl_GetItem(hWnd, nTab, &tci);
	assert(bRet != FALSE);

	// Determine the colors.
	const COLORREF colorActiveBg = g_darkBkColor * 3 / 2;
	const COLORREF colorInactiveBgBase = g_darkBkColor;
	const COLORREF colorActiveText = g_darkTextColor;
	const COLORREF colorInactiveText = RGB(192, 192, 192);

	HDC hDC = pDrawItemStruct->hDC;
	int nSavedDC = SaveDC(hDC);

	SetBkMode(hDC, TRANSPARENT);
	HBRUSH hBrush = CreateSolidBrush(colorInactiveBgBase);
	FillRect(hDC, &rect, hBrush);
	DeleteBrush(hBrush);

	// equalize drawing areas of active and inactive tabs
	// NOTE: Notepad++ had separate X and Y values, since Windows
	// technically supports non-square pixels (e.g. Hercules on
	// 16-bit Windows), but modern Windows only supports square.
	const int paddingDynamicTwo = rp_AdjustSizeForWindow(hWnd, 2);
	// Based on the Dark Mode path; for Light Mode, we use regular tabs.
	rect.left -= paddingDynamicTwo;
	rect.right += paddingDynamicTwo;
	//rect.top += paddingDynamicTwo;	// NOTE: Cancelled out below.
	rect.bottom += paddingDynamicTwo;

	// No multiple lines support.
	//rect.top -= paddingDynamicTwo;	// NOTE: Cancels out the above, so just commented out.

	// Draw highlights on tabs.
	if (isSelected) {
		hBrush = CreateSolidBrush(colorActiveBg);
		FillRect(hDC, &pDrawItemStruct->rcItem, hBrush);
		DeleteBrush(hBrush);
	}

	// NOTE: Not drawing the top bar of the tab like in Notepad++.
	// ...but we will draw *some* lines: 1px for inactive, 2px for active.
	// (using the edge color)
	{
		RECT barRect = rect;
		if (isSelected) {
			barRect.bottom = barRect.top + paddingDynamicTwo;
		} else {
			barRect.top += paddingDynamicTwo;
			barRect.bottom = barRect.top + (paddingDynamicTwo / 2);
		}

		if (!hbrEdge) {
			hbrEdge = CreateSolidBrush(colorEdge);
		}
		FillRect(hDC, &barRect, hbrEdge);
	}

	// Draw text.
	SelectFont(hDC, GetWindowFont(hWnd));
	SIZE charPixel;
	GetTextExtentPoint(hDC, _T(" "), 1, &charPixel);
	const int spaceUnit = charPixel.cx;

	TEXTMETRIC textMetrics;
	GetTextMetrics(hDC, &textMetrics);
	const int textHeight = textMetrics.tmHeight;
	const int textDescent = textMetrics.tmDescent;

	// This code will read in one character at a time and remove every first ampersand (&).
	// ex. If input "test && test &&& test &&&&" then output will be "test & test && test &&&".
	// Tab's caption must be encoded like this because otherwise tab control would make tab too small or too big for the text.
	TCHAR decodedLabel[MAX_PATH];
	decodedLabel[0] = _T('\0');
	const TCHAR *in = label;
	TCHAR *out = decodedLabel;
	while (*in != _T('\0')) {
		if (*in == _T('&')) {
			while (*(++in) == '&') {
				*out++ = *in;
			}
		} else {
			*out++ = *in++;
		}
	}
	*out = '\0';

	// Center text vertically and horizontally
	const int Flags = DT_SINGLELINE | DT_NOPREFIX | DT_CENTER | DT_TOP;
	const int paddingText = ((pDrawItemStruct->rcItem.bottom - pDrawItemStruct->rcItem.top) - (textHeight + textDescent)) / 2;
	const int paddingDescent = (textDescent / 2) - (isSelected ? paddingDynamicTwo : 0);	// NOTE: Changed from NPP.
	rect.top = pDrawItemStruct->rcItem.top + paddingText + paddingDescent;
	rect.bottom = pDrawItemStruct->rcItem.bottom - paddingText + paddingDescent;
	rect.bottom -= (paddingDynamicTwo / 2);	// text is too low...

	// isDarkMode || !isSelected || _drawTopBar
	rect.top += paddingDynamicTwo;

	const COLORREF textColor = isSelected ? colorActiveText : colorInactiveText;
	SetTextColor(hDC, textColor);
	DrawText(hDC, decodedLabel, _tcslen(decodedLabel), &rect, Flags);
	RestoreDC(hDC, nSavedDC);
}

/**
 * Subclass procedure for Tab controls.
 * @param hWnd
 * @param uMsg
 * @param wParam
 * @param lParam
 * @param uIdSubclass
 * @param dwRefData Pointer to Dark Mode background brush.
 */
LRESULT WINAPI NppDarkMode_TabControlSubclassProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg) {
		case WM_DRAWITEM: {
			if (!g_darkModeEnabled)
				break;
			if (!(GetWindowLongPtr(hWnd, GWL_STYLE) & TCS_OWNERDRAWFIXED))
				break;

			NppDarkMode_TabControl_drawItem(hWnd, reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
			return TRUE;
		}

		case WM_ERASEBKGND: {
			if (!g_darkModeEnabled)
				break;
			if (!(GetWindowLongPtr(hWnd, GWL_STYLE) & TCS_OWNERDRAWFIXED))
				break;

			RECT rc;
			GetClientRect(hWnd, &rc);

			HBRUSH *pHbrBkgnd = reinterpret_cast<HBRUSH*>(dwRefData);
			if (!*pHbrBkgnd)
				*pHbrBkgnd = CreateSolidBrush(g_darkBkColor);

			FillRect(reinterpret_cast<HDC>(wParam), &rc, *pHbrBkgnd);
			return TRUE;
		}

		case WM_PAINT: {
			if (!g_darkModeEnabled)
				break;
			if (!(GetWindowLongPtr(hWnd, GWL_STYLE) & TCS_OWNERDRAWFIXED))
				break;

			// NOTE: Not handling anything fancy like multiple lines.

			// Draw the background.
			HBRUSH *pHbrBkgnd = reinterpret_cast<HBRUSH*>(dwRefData);
			if (!*pHbrBkgnd)
				*pHbrBkgnd = CreateSolidBrush(g_darkBkColor);

			PAINTSTRUCT ps;
			HDC hDC = BeginPaint(hWnd, &ps);
			FillRect(hDC, &ps.rcPaint, *pHbrBkgnd);

			const UINT id = GetDlgCtrlID(hWnd);

			if (!hpenEdge) {
				// TODO: DPI adjustments?
				hpenEdge = CreatePen(PS_SOLID, 1, colorEdge);
			}
			HPEN holdPen = static_cast<HPEN>(SelectObject(hDC, hpenEdge));

			// Clipping region
			HRGN holdClip = CreateRectRgn(0, 0, 0, 0);
			if (GetClipRgn(hDC, holdClip) != 1) {
				// Unable to get the clipping region.
				DeleteObject(holdClip);
				holdClip = nullptr;
			}

			// NOTE: Notepad++ had separate X and Y values, since Windows
			// technically supports non-square pixels (e.g. Hercules on
			// 16-bit Windows), but modern Windows only supports square.
			const int paddingDynamicTwo = rp_AdjustSizeForWindow(hWnd, 2);

			const int nTabs = TabCtrl_GetItemCount(hWnd);
			const int nFocusTab = TabCtrl_GetCurFocus(hWnd);
			const int nSelTab = TabCtrl_GetCurSel(hWnd);

			// Draw the right edge lines of each tab.
			// TODO: Adjust edge line width for DPI?
			for (int i = 0; i < nTabs; i++) {
				DRAWITEMSTRUCT dis = { ODT_TAB, id, static_cast<UINT>(i), ODA_DRAWENTIRE, ODS_DEFAULT, hWnd, hDC, {}, 0 };
				TabCtrl_GetItemRect(hWnd, i, &dis.rcItem);
				// FIXME: GetItemRect is slightly too small.
				dis.rcItem.top = 0;	// may be 2; subtracting paddingDynamicTwo only works at 96dpi
				dis.rcItem.bottom += (paddingDynamicTwo / 2);

				// Determine if this tab is focused and/or selected.
				if (i == nFocusTab) {
					dis.itemState |= ODS_FOCUS;
				}
				if (i == nSelTab) {
					dis.itemState |= ODS_SELECTED;
				}
				dis.itemState |= ODS_NOFOCUSRECT;

				RECT rcIntersect;
				if (IntersectRect(&rcIntersect, &ps.rcPaint, &dis.rcItem)) {
					// Rectangles intersect.
					POINT edges[] = {
						{dis.rcItem.right - 1, dis.rcItem.top},
						{dis.rcItem.right - 1, dis.rcItem.bottom}
					};
					if (i != nSelTab && (i != nSelTab - 1)) {
						edges[0].y += paddingDynamicTwo;
					}

					Polyline(hDC, edges, _countof(edges));
					dis.rcItem.right--;
				}

				HRGN hClip = CreateRectRgnIndirect(&dis.rcItem);
				SelectClipRgn(hDC, hClip);
				NppDarkMode_TabControl_drawItem(hWnd, &dis);
				DeleteObject(hClip);
				SelectClipRgn(hDC, holdClip);
			}

			// Left edge of the first tab.
			{
				RECT rcFirstTab;
				TabCtrl_GetItemRect(hWnd, 0, &rcFirstTab);
				rcFirstTab.top = 0;	// may be 2; subtracting paddingDynamicTwo only works at 96dpi
				rcFirstTab.bottom += (paddingDynamicTwo / 2);
				POINT edges[] = {
					{rcFirstTab.left, rcFirstTab.top},
					{rcFirstTab.left, rcFirstTab.bottom}
				};
				if (nSelTab != 0) {
					edges[0].y += paddingDynamicTwo;
				}

				Polyline(hDC, edges, _countof(edges));
			}

			// Draw the tab control border.
			RECT rcFrame, rcItem;
			GetClientRect(hWnd, &rcFrame);
			TabCtrl_GetItemRect(hWnd, 0, &rcItem);
			rcFrame.top += (rcItem.bottom - rcItem.top);
			rcFrame.left += rcItem.left;	// to match light mode
			rcFrame.right -= rcItem.left;	// to match light mode
			rcFrame.top += paddingDynamicTwo;
			if (!hbrEdge) {
				hbrEdge = CreateSolidBrush(colorEdge);
			}
			// TODO: Adjust frame width for DPI?
			FrameRect(hDC, &rcFrame, hbrEdge);

			SelectClipRgn(hDC, holdClip);
			if (holdClip) {
				DeleteObject(holdClip);
			}

			SelectObject(hDC, holdPen);
			EndPaint(hWnd, &ps);
			return FALSE;
		}

		case WM_SETTINGCHANGE:
			if (g_darkModeSupported && IsColorSchemeChangeMessage(lParam)) {
				SendMessage(hWnd, WM_THEMECHANGED, 0, 0);
			}
			break;

		case WM_THEMECHANGED: {
			if (!g_darkModeSupported)
				break;

			const LONG_PTR lpOldStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
			LONG_PTR lpStyle = lpOldStyle;
			if (g_darkModeEnabled) {
				lpStyle |= TCS_OWNERDRAWFIXED;
			} else {
				lpStyle &= ~TCS_OWNERDRAWFIXED;
			}

			if (lpStyle != lpOldStyle) {
				SetWindowLongPtr(hWnd, GWL_STYLE, lpStyle);
				// TODO: InvalidateRect()?
			}
			break;
		}

		case WM_DESTROY:
		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, NppDarkMode_TabControlSubclassProc, uIdSubclass);
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
