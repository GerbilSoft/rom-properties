/**
 * Custom subclasses for full Dark Mode functionality.
 *
 * Based on TortoiseGit's dark mode theme:
 * https://gitlab.com/tortoisegit/tortoisegit/-/blob/HEAD/src/Utils/Theme.cpp
 */

#include "TGDarkMode.hpp"
#include "DarkMode.hpp"
#include "DarkModeCtrl.hpp"	// for dark mode colors

#include <tchar.h>

// Theming constants
#if _WIN32_WINNT >= 0x0600
#  include <vssym32.h>
#else
#  include <tmschema.h>
#endif

// gdiplus
// NOTE: Gdiplus requires min/max.
#include <algorithm>
namespace Gdiplus {
        using std::min;
        using std::max;
}
#include <olectl.h>
#include <gdiplus.h>

// C includes (C++ namespace)
#include <cassert>

// C++ STL classes
#include <memory>
using std::make_unique;

#ifndef RECTWIDTH
#  define RECTWIDTH(rc) ((rc).right - (rc).left)
#endif

#ifndef RECTHEIGHT
#  define RECTHEIGHT(rc) ((rc).bottom - (rc).top)
#endif

// DPI-aware functions.
typedef UINT (WINAPI *fnGetDpiForWindow)(HWND hWnd);
typedef UINT (WINAPI *fnGetDpiForSystem)(void);
static fnGetDpiForWindow _GetDpiForWindow = nullptr;
static fnGetDpiForSystem _GetDpiForSystem = nullptr;
static bool m_dpi_init = false;

// Reference: https://gitlab.com/tortoisegit/tortoisegit/-/blob/HEAD/src/Utils/DPIAware.h
static int GetDPI(HWND hWnd)
{
	if (!m_dpi_init) {
		// Initialize DPI functions.
		HMODULE hUser32 = GetModuleHandle(_T("user32.dll"));
		if (hUser32) {
			_GetDpiForWindow = reinterpret_cast<fnGetDpiForWindow>(GetProcAddress(hUser32, "GetDpiForWindow"));
			_GetDpiForSystem = reinterpret_cast<fnGetDpiForSystem>(GetProcAddress(hUser32, "GetDpiForSystem"));
		}

		// NOTE: TortoiseGit caches the system DPI, but MSDN says we shouldn't.
		// Reference: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdpiforsystem
		m_dpi_init = true;
	}

	if (_GetDpiForWindow && hWnd) {
		return _GetDpiForWindow(hWnd);
	}

	// Window-specific DPI function not found.
	// Return the system DPI if it's available.
	if (_GetDpiForSystem) {
		return _GetDpiForSystem();
	}

	// System-specific DPI function not found.
	// Get the DPI from the system DC.
	HDC hDC = GetDC(nullptr);
	if (hDC) {
		// TODO: What if non-square pixels show up again?
		const int dpi = GetDeviceCaps(hDC, LOGPIXELSX);
		ReleaseDC(nullptr, hDC);
		return dpi;
	}

	// Unable to get the DPI.
	// Use a "standard" default value.
	return 96;
}

static int GetDPIX(HWND hWnd) { return GetDPI(hWnd); }
static int GetDPIY(HWND hWnd) { return GetDPI(hWnd); }

/**
 * Get state from a BUTTON control's state.
 * @param dwStyle
 * @param bHot
 * @param bFocus
 * @param dwCheckState
 * @param iPartId
 * @param bHasMouseCapture
 * @return
 */
static int GetStateFromBtnState(LONG_PTR dwStyle, BOOL bHot, BOOL bFocus, LRESULT dwCheckState, int iPartId, BOOL bHasMouseCapture)
{
	int iState = 0;
	switch (iPartId) {
		case BP_PUSHBUTTON:
			iState = PBS_NORMAL;
			if (dwStyle & WS_DISABLED)
				iState = PBS_DISABLED;
			else {
				if (dwStyle & BS_DEFPUSHBUTTON)
					iState = PBS_DEFAULTED;

				if (bHasMouseCapture && bHot)
					iState = PBS_PRESSED;
				else if (bHasMouseCapture || bHot)
					iState = PBS_HOT;
			}
			break;
		case BP_GROUPBOX:
			iState = (dwStyle & WS_DISABLED) ? GBS_DISABLED : GBS_NORMAL;
			break;

		case BP_RADIOBUTTON:
			iState = RBS_UNCHECKEDNORMAL;
			switch (dwCheckState) {
				case BST_CHECKED:
					if (dwStyle & WS_DISABLED)
						iState = RBS_CHECKEDDISABLED;
					else if (bFocus)
						iState = RBS_CHECKEDPRESSED;
					else if (bHot)
						iState = RBS_CHECKEDHOT;
					else
						iState = RBS_CHECKEDNORMAL;
					break;
				case BST_UNCHECKED:
					if (dwStyle & WS_DISABLED)
						iState = RBS_UNCHECKEDDISABLED;
					else if (bFocus)
						iState = RBS_UNCHECKEDPRESSED;
					else if (bHot)
						iState = RBS_UNCHECKEDHOT;
					else
						iState = RBS_UNCHECKEDNORMAL;
					break;
			}
			break;

		case BP_CHECKBOX:
			switch (dwCheckState) {
				case BST_CHECKED:
					if (dwStyle & WS_DISABLED)
						iState = CBS_CHECKEDDISABLED;
					else if (bFocus)
						iState = CBS_CHECKEDPRESSED;
					else if (bHot)
						iState = CBS_CHECKEDHOT;
					else
						iState = CBS_CHECKEDNORMAL;
					break;
				case BST_INDETERMINATE:
					if (dwStyle & WS_DISABLED)
						iState = CBS_MIXEDDISABLED;
					else if (bFocus)
						iState = CBS_MIXEDPRESSED;
					else if (bHot)
						iState = CBS_MIXEDHOT;
					else
						iState = CBS_MIXEDNORMAL;
					break;
				case BST_UNCHECKED:
					if (dwStyle & WS_DISABLED)
						iState = CBS_UNCHECKEDDISABLED;
					else if (bFocus)
						iState = CBS_UNCHECKEDPRESSED;
					else if (bHot)
						iState = CBS_UNCHECKEDHOT;
					else
						iState = CBS_UNCHECKEDNORMAL;
					break;
			}
			break;
		default:
			assert(0);
			break;
	}

	return iState;
}

static void GetRoundRectPath(Gdiplus::GraphicsPath* pPath, const Gdiplus::Rect& r, int dia)
{
	// diameter can't exceed width or height
	if (dia > r.Width)
		dia = r.Width;
	if (dia > r.Height)
		dia = r.Height;

	// define a corner
	Gdiplus::Rect Corner(r.X, r.Y, dia, dia);

	// begin path
	pPath->Reset();
	pPath->StartFigure();

	// top left
	pPath->AddArc(Corner, 180, 90);

	// top right
	Corner.X += (r.Width - dia - 1);
	pPath->AddArc(Corner, 270, 90);

	// bottom right
	Corner.Y += (r.Height - dia - 1);
	pPath->AddArc(Corner, 0, 90);

	// bottom left
	Corner.X -= (r.Width - dia - 1);
	pPath->AddArc(Corner, 90, 90);

	// end path
	pPath->CloseFigure();
}

static void DrawRect(LPRECT prc, HDC hdcPaint, Gdiplus::DashStyle dashStyle, Gdiplus::Color clr, Gdiplus::REAL width)
{
	auto myPen = std::make_unique<Gdiplus::Pen>(clr, width);
	myPen->SetDashStyle(dashStyle);
	auto myGraphics = std::make_unique<Gdiplus::Graphics>(hdcPaint);

	myGraphics->DrawRectangle(myPen.get(), static_cast<INT>(prc->left), static_cast<INT>(prc->top), static_cast<INT>(prc->right - 1 - prc->left), static_cast<INT>(prc->bottom - 1 - prc->top));
}

static inline void DrawFocusRect(LPRECT prcFocus, HDC hdcPaint)
{
	DrawRect(prcFocus, hdcPaint, Gdiplus::DashStyleDot, Gdiplus::Color(0xFF, 0, 0, 0), 1.0);
}

/**
 * Paint the control.
 * @param hWnd
 * @param hdc
 * @param prc
 * @param bDrawBorder
 */
static void PaintControl(HWND hWnd, HDC hdc, RECT* prc, bool bDrawBorder)
{
	HDC hdcPaint = nullptr;

	if (bDrawBorder)
		InflateRect(prc, 1, 1);

	HPAINTBUFFER hBufferedPaint = _BeginBufferedPaint(hdc, prc, BPBF_TOPDOWNDIB, nullptr, &hdcPaint);
	if (hdcPaint && hBufferedPaint) {
		RECT rc;
		GetWindowRect(hWnd, &rc);

		PatBlt(hdcPaint, 0, 0, RECTWIDTH(rc), RECTHEIGHT(rc), BLACKNESS);
		_BufferedPaintSetAlpha(hBufferedPaint, &rc, 0x00);

		///
		/// first blit white so list ctrls don't look ugly:
		///
		PatBlt(hdcPaint, 0, 0, RECTWIDTH(rc), RECTHEIGHT(rc), WHITENESS);

		if (bDrawBorder)
			InflateRect(prc, -1, -1);
		// Tell the control to paint itself in our memory buffer
		SendMessage(hWnd, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(hdcPaint), PRF_CLIENT | PRF_ERASEBKGND | PRF_NONCLIENT | PRF_CHECKVISIBLE);

		if (bDrawBorder) {
			InflateRect(prc, 1, 1);
			FrameRect(hdcPaint, prc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
		}

		// don't make a possible border opaque, only the inner part of the control
		InflateRect(prc, -2, -2);
		// Make every pixel opaque
		_BufferedPaintSetAlpha(hBufferedPaint, prc, 255);
		_EndBufferedPaint(hBufferedPaint, TRUE);
	}
}

/**
 * Determine glow size.
 * @param piSize
 * @param pszClassIdList
 */
static BOOL DetermineGlowSize(int* piSize, LPCWSTR pszClassIdList = nullptr)
{
	if (!piSize) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!pszClassIdList)
		pszClassIdList = L"CompositedWindow::Window";

	if (HTHEME hThemeWindow = _OpenThemeData(nullptr, pszClassIdList)) {
		SUCCEEDED(_GetThemeInt(hThemeWindow, 0, 0, TMT_TEXTGLOWSIZE, piSize));
		_CloseThemeData(hThemeWindow);
		return TRUE;
	}

	SetLastError(ERROR_FILE_NOT_FOUND);
	return FALSE;
}

/**
 * Get an Edit control's border color.
 * @param hWnd
 * @param pClr
 */
static BOOL GetEditBorderColor(HWND hWnd, COLORREF* pClr)
{
	assert(pClr);

	if (HTHEME hTheme = _OpenThemeData(hWnd, L"Edit")) {
		_GetThemeColor(hTheme, EP_BACKGROUNDWITHBORDER, EBWBS_NORMAL, TMT_BORDERCOLOR, pClr);
		_CloseThemeData(hTheme);
		return TRUE;
	}

	return FALSE;
}


/**
 * Subclass procedure for Button controls.
 * @param hWnd
 * @param uMsg
 * @param wParam
 * @param lParam
 * @param uIdSubclass
 * @param dwRefData
 */
LRESULT WINAPI TGDarkMode_ButtonSubclassProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (!g_darkModeEnabled) {
		// Using light mode. Don't bother with any of this.
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
		case WM_SETTEXT:
		case WM_ENABLE:
		case WM_STYLECHANGED: {
			LRESULT res = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			InvalidateRgn(hWnd, nullptr, FALSE);
			return res;
		}

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			if (!hdc)
				break;

			LONG_PTR dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
			LONG_PTR dwButtonStyle = LOWORD(dwStyle);
			LONG_PTR dwButtonType = dwButtonStyle & 0xF;
			RECT rcClient;
			GetClientRect(hWnd, &rcClient);

			if ((dwButtonType & BS_GROUPBOX) == BS_GROUPBOX) {
				if (HTHEME hTheme = _OpenThemeData(hWnd, L"Button")) {
					BP_PAINTPARAMS params = { sizeof(BP_PAINTPARAMS) };
					params.dwFlags = BPPF_ERASE;

					RECT rcExclusion = rcClient;
					params.prcExclude = &rcExclusion;

					// We have to calculate the exclusion rect and therefore
					// calculate the font height. We select the control's font
					// into the DC and fake a drawing operation:
					auto hFontOld = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0L, 0));
					if (hFontOld)
						hFontOld = static_cast<HFONT>(SelectObject(hdc, hFontOld));

					RECT rcDraw = rcClient;
					DWORD dwFlags = DT_SINGLELINE;

					// we use uppercase A to determine the height of text, so we
					// can draw the upper line of the groupbox:
					DrawTextW(hdc, L"A", -1, &rcDraw, dwFlags | DT_CALCRECT);

					if (hFontOld) {
						SelectObject(hdc, hFontOld);
						hFontOld = nullptr;
					}

					rcExclusion.left += 2;
					rcExclusion.top += RECTHEIGHT(rcDraw);
					rcExclusion.right -= 2;
					rcExclusion.bottom -= 2;

					HDC hdcPaint = nullptr;
					HPAINTBUFFER hBufferedPaint = _BeginBufferedPaint(hdc, &rcClient, BPBF_TOPDOWNDIB, &params, &hdcPaint);
					if (hdcPaint) {
						// now we again retrieve the font, but this time we select it into
						// the buffered DC:
						hFontOld = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0L, 0));
						if (hFontOld)
							hFontOld = static_cast<HFONT>(SelectObject(hdcPaint, hFontOld));

						::SetBkColor(hdcPaint, g_darkBkColor);
						::ExtTextOut(hdcPaint, 0, 0, ETO_OPAQUE, &rcClient, nullptr, 0, nullptr);

						_BufferedPaintSetAlpha(hBufferedPaint, &ps.rcPaint, 0x00);

						DTTOPTS DttOpts = { sizeof(DTTOPTS) };
						DttOpts.dwFlags = DTT_COMPOSITED | DTT_GLOWSIZE;
						DttOpts.crText = g_darkTextColor;
						DttOpts.iGlowSize = 12; // Default value

						DetermineGlowSize(&DttOpts.iGlowSize);

						COLORREF cr = g_darkBkColor;
						GetEditBorderColor(hWnd, &cr);
						cr |= 0xff000000;

						auto myPen = std::make_unique<Gdiplus::Pen>(Gdiplus::Color(cr));
						auto myGraphics = std::make_unique<Gdiplus::Graphics>(hdcPaint);
						int iY = RECTHEIGHT(rcDraw) / 2;
						auto rr = Gdiplus::Rect(rcClient.left, rcClient.top + iY, RECTWIDTH(rcClient), RECTHEIGHT(rcClient) - iY - 1);
						Gdiplus::GraphicsPath path;
						GetRoundRectPath(&path, rr, 5);
						myGraphics->DrawPath(myPen.get(), &path);
						myGraphics.reset();
						myPen.reset();

						int iLen = GetWindowTextLength(hWnd);
						if (iLen) {
							iLen += 5; // 1 for terminating zero, 4 for DT_MODIFYSTRING
							auto szText = static_cast<LPWSTR>(LocalAlloc(LPTR, sizeof(WCHAR) * iLen));
							if (szText) {
								iLen = GetWindowTextW(hWnd, szText, iLen);
								if (iLen) {
									int iX = RECTWIDTH(rcDraw);
									rcDraw = rcClient;
									rcDraw.left += iX;
									DrawTextW(hdcPaint, szText, -1, &rcDraw, dwFlags | DT_CALCRECT);
									::SetBkColor(hdcPaint, g_darkBkColor);
									::ExtTextOut(hdcPaint, 0, 0, ETO_OPAQUE, &rcDraw, nullptr, 0, nullptr);
									++rcDraw.left;
									++rcDraw.right;

									SetBkMode(hdcPaint, TRANSPARENT);
									SetTextColor(hdcPaint, g_darkTextColor);
									DrawText(hdcPaint, szText, -1, &rcDraw, dwFlags);
								}
								LocalFree(szText);
							}
						}

						if (hFontOld) {
							SelectObject(hdcPaint, hFontOld);
							hFontOld = nullptr;
						}

						_EndBufferedPaint(hBufferedPaint, TRUE);
					}
					_CloseThemeData(hTheme);
				}
			}

			else if (dwButtonType == BS_CHECKBOX || dwButtonType == BS_AUTOCHECKBOX ||
				 dwButtonType == BS_3STATE || dwButtonType == BS_AUTO3STATE || dwButtonType == BS_RADIOBUTTON || dwButtonType == BS_AUTORADIOBUTTON)
			{
				if (HTHEME hTheme = _OpenThemeData(hWnd, L"Button")) {
					HDC hdcPaint = nullptr;
					BP_PAINTPARAMS params = { sizeof(BP_PAINTPARAMS) };
					params.dwFlags = BPPF_ERASE;
					HPAINTBUFFER hBufferedPaint = _BeginBufferedPaint(hdc, &rcClient, BPBF_TOPDOWNDIB, &params, &hdcPaint);
					if (hdcPaint && hBufferedPaint) {
						::SetBkColor(hdcPaint, g_darkBkColor);
						::ExtTextOut(hdcPaint, 0, 0, ETO_OPAQUE, &rcClient, nullptr, 0, nullptr);

						_BufferedPaintSetAlpha(hBufferedPaint, &ps.rcPaint, 0x00);

						LRESULT dwCheckState = SendMessage(hWnd, BM_GETCHECK, 0, 0);
						POINT pt;
						RECT rc;
						GetWindowRect(hWnd, &rc);
						GetCursorPos(&pt);
						BOOL bHot = PtInRect(&rc, pt);
						BOOL bFocus = GetFocus() == hWnd;

						int iPartId = BP_CHECKBOX;
						if (dwButtonType == BS_RADIOBUTTON || dwButtonType == BS_AUTORADIOBUTTON)
							iPartId = BP_RADIOBUTTON;

						int iState = GetStateFromBtnState(dwStyle, bHot, bFocus, dwCheckState, iPartId, FALSE);
						int bmWidth = int(ceil(13.0 * GetDPIX(hWnd) / 96.0));
						UINT uiHalfWidth = (RECTWIDTH(rcClient) - bmWidth) / 2;

						// we have to use the whole client area, otherwise we get only partially
						// drawn areas:
						RECT rcPaint = rcClient;

						if (dwButtonStyle & BS_LEFTTEXT) {
							rcPaint.left += uiHalfWidth;
							rcPaint.right += uiHalfWidth;
						} else {
							rcPaint.left -= uiHalfWidth;
							rcPaint.right -= uiHalfWidth;
						}

						// we assume that bmWidth is both the horizontal and the vertical
						// dimension of the control bitmap and that it is square. bm.bmHeight
						// seems to be the height of a striped bitmap because it is an absurdly
						// high dimension value
						if ((dwButtonStyle & BS_VCENTER) == BS_VCENTER) /// BS_VCENTER is BS_TOP|BS_BOTTOM
						{
							int h = RECTHEIGHT(rcPaint);
							rcPaint.top = (h - bmWidth) / 2;
							rcPaint.bottom = rcPaint.top + bmWidth;
						}
						else if (dwButtonStyle & BS_TOP)
							rcPaint.bottom = rcPaint.top + bmWidth;
						else if (dwButtonStyle & BS_BOTTOM)
							rcPaint.top = rcPaint.bottom - bmWidth;
						else // default: center the checkbox/radiobutton vertically
						{
							int h = RECTHEIGHT(rcPaint);
							rcPaint.top = (h - bmWidth) / 2;
							rcPaint.bottom = rcPaint.top + bmWidth;
						}

						_DrawThemeBackground(hTheme, hdcPaint, iPartId, iState, &rcPaint, nullptr);
						rcPaint = rcClient;

						_GetThemeBackgroundContentRect(hTheme, hdcPaint, iPartId, iState, &rcPaint, &rc);

						if (dwButtonStyle & BS_LEFTTEXT)
							rc.right -= bmWidth + 2 * GetSystemMetrics(SM_CXEDGE);
						else
							rc.left += bmWidth + 2 * GetSystemMetrics(SM_CXEDGE);

						DTTOPTS DttOpts = { sizeof(DTTOPTS) };
						DttOpts.dwFlags = DTT_COMPOSITED | DTT_GLOWSIZE;
						DttOpts.crText = g_darkTextColor;
						DttOpts.iGlowSize = 12; // Default value

						DetermineGlowSize(&DttOpts.iGlowSize);

						HFONT hFontOld = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0L, 0));
						if (hFontOld)
							hFontOld = static_cast<HFONT>(SelectObject(hdcPaint, hFontOld));

						int iLen = GetWindowTextLength(hWnd);
						if (iLen) {
							iLen += 5; // 1 for terminating zero, 4 for DT_MODIFYSTRING
							LPWSTR szText = static_cast<LPWSTR>(LocalAlloc(LPTR, sizeof(WCHAR) * iLen));
							if (szText) {
								iLen = GetWindowTextW(hWnd, szText, iLen);
								if (iLen) {
									DWORD dwFlags = DT_SINGLELINE /*|DT_VCENTER*/;
									if (dwButtonStyle & BS_MULTILINE) {
										dwFlags |= DT_WORDBREAK;
										dwFlags &= ~(DT_SINGLELINE | DT_VCENTER);
									}

									if ((dwButtonStyle & BS_CENTER) == BS_CENTER) /// BS_CENTER is BS_LEFT|BS_RIGHT
										dwFlags |= DT_CENTER;
									else if (dwButtonStyle & BS_LEFT)
										dwFlags |= DT_LEFT;
									else if (dwButtonStyle & BS_RIGHT)
										dwFlags |= DT_RIGHT;

									if ((dwButtonStyle & BS_VCENTER) == BS_VCENTER) /// BS_VCENTER is BS_TOP|BS_BOTTOM
										dwFlags |= DT_VCENTER;
									else if (dwButtonStyle & BS_TOP)
										dwFlags |= DT_TOP;
									else if (dwButtonStyle & BS_BOTTOM)
										dwFlags |= DT_BOTTOM;
									else
										dwFlags |= DT_VCENTER;

									if ((dwButtonStyle & BS_MULTILINE) && (dwFlags & DT_VCENTER)) {
										// the DT_VCENTER flag only works for DT_SINGLELINE, so
										// we have to center the text ourselves here
										RECT rcdummy = rc;
										int height = DrawText(hdcPaint, szText, -1, &rcdummy, dwFlags | DT_WORDBREAK | DT_CALCRECT);
										int center_y = rc.top + (RECTHEIGHT(rc) / 2);
										rc.top = center_y - height / 2;
										rc.bottom = center_y + height / 2;
									}
									SetBkMode(hdcPaint, TRANSPARENT);
									if (dwStyle & WS_DISABLED)
										SetTextColor(hdcPaint, g_darkDisabledTextColor);
									else
										SetTextColor(hdcPaint, g_darkTextColor);
									DrawText(hdcPaint, szText, -1, &rc, dwFlags);

									// draw the focus rectangle if neccessary:
									if (bFocus) {
										RECT rcDraw = rc;

										DrawTextW(hdcPaint, szText, -1, &rcDraw, dwFlags | DT_CALCRECT);
										if (dwFlags & DT_SINGLELINE) {
											dwFlags &= ~DT_VCENTER;
											RECT rcDrawTop;
											DrawTextW(hdcPaint, szText, -1, &rcDrawTop, dwFlags | DT_CALCRECT);
											rcDraw.top = rcDraw.bottom - RECTHEIGHT(rcDrawTop);
										}

										if (dwFlags & DT_RIGHT) {
											int iWidth = RECTWIDTH(rcDraw);
											rcDraw.right = rc.right;
											rcDraw.left = rcDraw.right - iWidth;
										}

										RECT rcFocus;
										IntersectRect(&rcFocus, &rc, &rcDraw);

										DrawFocusRect(&rcFocus, hdcPaint);
									}
								}
								LocalFree(szText);
							}
						}

						if (hFontOld) {
							SelectObject(hdcPaint, hFontOld);
							hFontOld = nullptr;
						}

						_EndBufferedPaint(hBufferedPaint, TRUE);
					}
					_CloseThemeData(hTheme);
				}
			}
			else if (BS_PUSHBUTTON == dwButtonType || BS_DEFPUSHBUTTON == dwButtonType) {
				// push buttons are drawn properly in dark mode without us doing anything
				return DefSubclassProc(hWnd, uMsg, wParam, lParam);
			}
			else
				PaintControl(hWnd, hdc, &ps.rcPaint, false);

			EndPaint(hWnd, &ps);
			return 0;
		}

		case WM_DESTROY:
		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, TGDarkMode_ButtonSubclassProc, uIdSubclass);
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/**
 * Subclass procedure for ComboBox(Ex) controls.
 * @param hWnd
 * @param uMsg
 * @param wParam
 * @param lParam
 * @param uIdSubclass
 * @param dwRefData
 */
LRESULT WINAPI TGDarkMode_ComboBoxSubclassProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (!g_darkModeEnabled) {
		// Using light mode. Don't bother with any of this.
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORSCROLLBAR: {
			auto pHbrBkgnd = reinterpret_cast<HBRUSH*>(dwRefData);
			HDC hdc = reinterpret_cast<HDC>(wParam);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, g_darkTextColor);
			SetBkColor(hdc, g_darkBkColor);
			if (!*pHbrBkgnd)
				*pHbrBkgnd = CreateSolidBrush(g_darkBkColor);
			return reinterpret_cast<LRESULT>(*pHbrBkgnd);
		}

		case WM_DRAWITEM: {
			auto pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
			HDC hDC = pDIS->hDC;
			RECT rc = pDIS->rcItem;
			wchar_t itemText[1024] = { 0 };

			COMBOBOXEXITEM cbi = { 0 };
			cbi.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY | CBEIF_INDENT;
			cbi.iItem = pDIS->itemID;
			cbi.cchTextMax = _countof(itemText);
			cbi.pszText = itemText;

			auto cwnd = GetParent(hWnd);
			if (SendMessage(cwnd, CBEM_GETITEM, 0, reinterpret_cast<LPARAM>(&cbi))) {
				rc.left += (cbi.iIndent * 10);
				if (pDIS->itemState & LVIS_FOCUSED)
					::SetBkColor(hDC, g_darkDisabledTextColor);
				else
					::SetBkColor(hDC, g_darkBkColor);
				::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, nullptr, 0, nullptr);

				if (cbi.mask & CBEIF_IMAGE) {
					auto imglist = reinterpret_cast<HIMAGELIST>(SendMessage(cwnd, CBEM_GETIMAGELIST, 0, 0));
					if (imglist) {
						const int img = (pDIS->itemState & LVIS_SELECTED) ? cbi.iSelectedImage : cbi.iImage;
						int iconX(0), iconY(0);
						ImageList_GetIconSize(imglist, &iconX, &iconY);
						ImageList_Draw(imglist, img, hDC, rc.left, rc.top, ILD_TRANSPARENT | INDEXTOOVERLAYMASK(cbi.iOverlay));
						rc.left += (iconX + 2);
					}
				}

				SetTextColor(pDIS->hDC, g_darkTextColor);
				SetBkMode(hDC, TRANSPARENT);
				DrawText(hDC, cbi.pszText, -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
				return TRUE;
			}
			break;
		}

		case WM_DESTROY:
		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, TGDarkMode_ComboBoxSubclassProc, uIdSubclass);
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
