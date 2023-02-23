/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * MessageWidget.cpp: Message widget. (Similar to KMessageWidget)          *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MessageWidget.hpp"

static ATOM atom_messageWidget;

#define IDC_CLOSE_BUTTON	0x0101
#define BORDER_SIZE 4

class MessageWidgetPrivate
{
	public:
		explicit MessageWidgetPrivate(HWND hWnd);
		~MessageWidgetPrivate();

	public:
		void setMessageType(unsigned int messageType)
		{
			messageType &= 0x70;
			if (this->messageType == messageType || messageType > 0x40)
				return;

			this->messageType = messageType;
			updateIcon();
		}

	private:
		/**
		 * Update the icon and brushes.
		 */
		void updateIcon();

	public:
		/** Window Message functions **/

		/**
		 * WM_PAINT handler.
		 */
		void paint(void);

	public:
		HWND hWnd;			// MessageWidget control
		HFONT hFont;			// set by the parent window
		HICON hIcon;			// loaded with LR_SHARED

		HFONT hFontMarlett;
		HFONT hFontMarlettBold;

		HBRUSH hbrBorder;		// Border brush
		HBRUSH hbrBg;			// Background brush
		COLORREF colorBg;		// Background color
		unsigned int messageType;	// MB_ICON*
		SIZE szIcon;			// Icon size

		enum CloseButtonState {
			CLSBTN_NORMAL = 0,
			CLSBTN_HOVER,
			CLSBTN_PRESSED,
		};
		CloseButtonState closeButtonState;
		RECT rectBtnClose;		// Close button rect
		bool bBtnCloseEntered;		// True if the mouse cursor entered the Close button area.
		bool bBtnCloseDown;		// True if WM_LBUTTONDOWN received while over the Close button.
};

MessageWidgetPrivate::MessageWidgetPrivate(HWND hWnd)
	: hWnd(hWnd)
	, hFont(nullptr)
	, hIcon(nullptr)
	, hFontMarlett(nullptr)
	, hFontMarlettBold(nullptr)
	, hbrBorder(nullptr)
	, hbrBg(nullptr)
	, colorBg(0)
	, messageType(MB_ICONINFORMATION)
	, closeButtonState(CLSBTN_NORMAL)
	, bBtnCloseEntered(false)
	, bBtnCloseDown(false)
{
	// TODO: Update szIcon on system DPI change.
	szIcon.cx = GetSystemMetrics(SM_CXSMICON);
	szIcon.cy = GetSystemMetrics(SM_CYSMICON);

	// Initialize the icon.
	updateIcon();

	// Close button positioning will be handled in WM_SIZE.
	memset(&rectBtnClose, 0, sizeof(rectBtnClose));

	// Create the fonts for the Close button.
	// (one regular, one bold)
	LOGFONT lfMarlett = {
		0,			// lfHeight
		12,			// lfWidth
		0,			// lfEscapement
		0,			// lfOrientation
		FW_NORMAL,		// lfWeight
		FALSE,			// lfItalic
		FALSE,			// lfUnderline
		FALSE,			// lfStrikeout
		DEFAULT_CHARSET,	// lfCharset
		OUT_DEFAULT_PRECIS,	// lfOutPrecision
		CLIP_DEFAULT_PRECIS,	// lfClipPrecision
		DEFAULT_QUALITY,	// lfQuality
		DEFAULT_PITCH | FF_DONTCARE,	// lfPitchAndFamily
		_T("Marlett"),		// lfFaceName
	};
	hFontMarlett = CreateFontIndirect(&lfMarlett);
	lfMarlett.lfWeight = FW_BOLD;
	hFontMarlettBold = CreateFontIndirect(&lfMarlett);
}

MessageWidgetPrivate::~MessageWidgetPrivate()
{
	if (hFontMarlett) {
		DeleteFont(hFontMarlett);
	}
	if (hFontMarlettBold) {
		DeleteFont(hFontMarlettBold);
	}
	if (hbrBorder) {
		DeleteBrush(hbrBorder);
	}
	if (hbrBg) {
		DeleteBrush(hbrBg);
	}
}

/**
 * Update the icon and brushes.
 */
void MessageWidgetPrivate::updateIcon(void)
{
	if (hbrBorder) {
		DeleteBrush(hbrBorder);
		hbrBorder = nullptr;
	}
	if (hbrBg) {
		DeleteBrush(hbrBg);
		hbrBg = nullptr;
	}

	LPCTSTR lpszRes;
	switch (messageType) {
		case 0:
			lpszRes = nullptr;
			break;
		case MB_ICONEXCLAMATION:
			lpszRes = MAKEINTRESOURCE(101);
			hbrBorder = CreateSolidBrush(0x0074F6);
			colorBg = 0x419BFF;
			break;
		case MB_ICONQUESTION:
			lpszRes = MAKEINTRESOURCE(102);
			hbrBorder = CreateSolidBrush(0xE9AE3D);
			colorBg = 0xFFD37F;
			break;
		case MB_ICONSTOP:
			lpszRes = MAKEINTRESOURCE(103);
			hbrBorder = CreateSolidBrush(0x5344DA);
			colorBg = 0x8A7EF7;
			break;
		case MB_ICONINFORMATION:
		default:
			lpszRes = MAKEINTRESOURCE(104);
			hbrBorder = CreateSolidBrush(0xE9AE3D);
			colorBg = 0xFFD37F;
			break;
	}
	if (lpszRes) {
		HMODULE hUser32 = GetModuleHandle(_T("user32"));
		assert(hUser32 != nullptr);
		if (hUser32) {
			hbrBg = CreateSolidBrush(colorBg);
			hIcon = (HICON)LoadImage(hUser32, lpszRes, IMAGE_ICON,
				szIcon.cx, szIcon.cy, LR_SHARED);
		} else {
			hIcon = nullptr;
		}
	} else {
		hIcon = nullptr;
	}

	// Invalidate the entire control.
	InvalidateRect(hWnd, nullptr, TRUE);
}

/**
 * WM_PAINT handler.
 */
void MessageWidgetPrivate::paint(void)
{
	RECT rect, rectUpdate;
	GetClientRect(hWnd, &rect);
	BOOL bHasUpdateRect = GetUpdateRect(hWnd, &rectUpdate, TRUE);

	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hWnd, &ps);
	SelectObject(hDC, hFont);
	SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));

	if (bHasUpdateRect && !EqualRect(&rectUpdate, &rectBtnClose)) {
		SetBkMode(hDC, TRANSPARENT);

		// Clear the background so we don't end up drawing over
		// the previous icon/text.
		bool bClearedBG = true;
		FillRect(hDC, &rect, hbrBorder);
		if (hbrBg) {
			RECT rectBg = rect;
			InflateRect(&rectBg, -(BORDER_SIZE/2), -(BORDER_SIZE/2));
			FillRect(hDC, &rectBg, hbrBg);
		}

		if (hIcon) {
			DrawIconEx(hDC, BORDER_SIZE, BORDER_SIZE, hIcon, szIcon.cx, szIcon.cy, 0, nullptr, DI_NORMAL);
			rect.left += szIcon.cx + BORDER_SIZE*2;
		} else {
			rect.left += BORDER_SIZE;
		}

		// Message
		RECT textRect = {
			rect.left, rect.top + (szIcon.cy / 4),
			// NOTE: Not subtracting 2x szIcon.cy in order to
			// leave room for descenders, e.g. in 'g' and 'y'.
			rect.right, rect.bottom - (szIcon.cy / 4)
		};
		TCHAR tbuf[128];
		int len = GetWindowTextLength(hWnd);
		if (len < 128) {
			GetWindowText(hWnd, tbuf, _countof(tbuf));
			DrawText(hDC, tbuf, len, &textRect, 0);
		} else {
			TCHAR *const tmbuf = static_cast<TCHAR*>(malloc((len+1) * sizeof(TCHAR)));
			GetWindowText(hWnd, tmbuf, len+1);
			DrawText(hDC, tmbuf, len, &textRect, 0);
			free(tmbuf);
		}
	} else {
		// Only updating the Close button.
		// Use OPAQUE background drawing.
		SetBkMode(hDC, OPAQUE);
		SetBkColor(hDC, colorBg);
	}

	// Close button
	RECT tmpRectBtnClose = rectBtnClose;
	switch (closeButtonState) {
		case CLSBTN_NORMAL:
		default:
			SelectObject(hDC, hFontMarlett);
			break;
		case CLSBTN_HOVER:
			SelectObject(hDC, hFontMarlettBold);
			break;
		case CLSBTN_PRESSED:
			SelectObject(hDC, hFontMarlettBold);
			tmpRectBtnClose.left += 2;
			tmpRectBtnClose.top += 2;
			break;
	}
	// "r" == Close button
	DrawText(hDC, _T("r "), 1, &tmpRectBtnClose, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	EndPaint(hWnd, &ps);
}

/** MessageWidget **/

static LRESULT CALLBACK
MessageWidgetWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// FIXME: Don't use GWLP_USERDATA; use extra window bytes?
	switch (uMsg) {
		default:
			break;

		case WM_NCCREATE: {
			MessageWidgetPrivate *const d = new MessageWidgetPrivate(hWnd);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(d));
			break;
		}

		case WM_NCDESTROY: {
			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			delete d;
			break;
		}

		case WM_SETTEXT: {
			// TODO: Don't invalidate the icon section.
			InvalidateRect(hWnd, nullptr, TRUE);
			break;
		}

		case WM_ERASEBKGND:
			// Handled by WM_PAINT.
			// TODO: Return FALSE if we're using "no message type"?
			return TRUE;

		case WM_PAINT: {
			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			d->paint();
			return TRUE;
		}

		case WM_SETFONT: {
			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			d->hFont = reinterpret_cast<HFONT>(wParam);
			if (LOWORD(lParam)) {
				InvalidateRect(hWnd, nullptr, TRUE);
			}
			return FALSE;
		}

		case WM_GETFONT: {
			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			return reinterpret_cast<LRESULT>(d->hFont);
		}

		case WM_SIZE: {
			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));

			// Invalidate the current Close button rect.
			InvalidateRect(hWnd, &d->rectBtnClose, TRUE);

			// Get the new rect for the Close button.
			RECT rect;
			GetClientRect(hWnd, &rect);

			SIZE szBtnClose;
			szBtnClose.cx = d->szIcon.cx + BORDER_SIZE;
			szBtnClose.cy = d->szIcon.cy + BORDER_SIZE;
			d->rectBtnClose.left = rect.right - szBtnClose.cx - BORDER_SIZE;
			d->rectBtnClose.top = (rect.bottom - szBtnClose.cy) / 2;
			d->rectBtnClose.right = d->rectBtnClose.left + szBtnClose.cx;
			d->rectBtnClose.bottom = d->rectBtnClose.top + szBtnClose.cy;

			// Invalidate the new Close button rect.
			InvalidateRect(hWnd, &d->rectBtnClose, TRUE);
			break;
		}

		case WM_MOUSEMOVE: {
			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!(wParam & MK_LBUTTON)) {
				d->bBtnCloseDown = false;
			}

			const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			MessageWidgetPrivate::CloseButtonState clsbtn;
			if (PtInRect(&d->rectBtnClose, pt)) {
				// We're hovering over the Close button.
				clsbtn = d->bBtnCloseDown
					? MessageWidgetPrivate::CLSBTN_PRESSED
					: MessageWidgetPrivate::CLSBTN_HOVER;
				if (!d->bBtnCloseEntered) {
					// Start mouse tracking for WM_MOUSELEAVE.
					TRACKMOUSEEVENT tme;
					tme.cbSize = sizeof(tme);
					tme.hwndTrack = hWnd;
					tme.dwFlags = TME_LEAVE;
					tme.dwHoverTime = 0;
					TrackMouseEvent(&tme);
					d->bBtnCloseEntered = true;
				}
			} else {
				// Not hovering over the Close button.
				clsbtn = MessageWidgetPrivate::CLSBTN_NORMAL;
			}
			if (clsbtn != d->closeButtonState) {
				d->closeButtonState = clsbtn;
				InvalidateRect(hWnd, &d->rectBtnClose, TRUE);
			}
			break;
		}

		case WM_LBUTTONDOWN: {
			if (wParam != MK_LBUTTON)
				break;

			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			if (PtInRect(&d->rectBtnClose, pt)) {
				// Mouse button down on the Close button.
				d->bBtnCloseDown = true;
				SetCapture(hWnd);

				// Redraw the Close button.
				d->closeButtonState = MessageWidgetPrivate::CLSBTN_PRESSED;
				InvalidateRect(hWnd, &d->rectBtnClose, TRUE);
				return TRUE;
			}
			break;
		}

		case WM_LBUTTONUP: {
			ReleaseCapture();
			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			if (d->bBtnCloseDown) {
				// Mouse button released over the Close button.
				d->bBtnCloseDown = false;
				if (d->closeButtonState != MessageWidgetPrivate::CLSBTN_NORMAL) {
					d->closeButtonState = MessageWidgetPrivate::CLSBTN_NORMAL;
					InvalidateRect(hWnd, &d->rectBtnClose, TRUE);
				}

				if (PtInRect(&d->rectBtnClose, pt)) {
					// Hide the widget.
					ShowWindow(hWnd, SW_HIDE);
					// Send a notification.
					const NMHDR nmhdr = {
						hWnd,				// hwndFrom
						(UINT_PTR)GetDlgCtrlID(hWnd),	// idFrom
						MSGWN_CLOSED,
					};
					SendMessage(GetParent(hWnd), WM_NOTIFY, nmhdr.idFrom, reinterpret_cast<LPARAM>(&nmhdr));
				}
				return TRUE;
			}
			break;
		}

		case WM_MOUSELEAVE: {
			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			d->bBtnCloseEntered = false;
			if (d->closeButtonState != MessageWidgetPrivate::CLSBTN_NORMAL) {
				d->closeButtonState = MessageWidgetPrivate::CLSBTN_NORMAL;
				InvalidateRect(hWnd, &d->rectBtnClose, TRUE);
			}
			return TRUE;
		}

		case WM_MSGW_SET_MESSAGE_TYPE: {
			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			d->setMessageType(static_cast<unsigned int>(wParam));
			return TRUE;
		}

		case WM_MSGW_GET_MESSAGE_TYPE: {
			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			return d->messageType;
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void MessageWidgetRegister(void)
{
	if (atom_messageWidget != 0)
		return;

	static const WNDCLASSEX wndClass = {
		sizeof(WNDCLASSEX),		// cbSize
		CS_HREDRAW | CS_VREDRAW,	// style
		MessageWidgetWndProc,		// lpfnWndProc
		0,				// cbClsExtra
		0,				// cbWndExtra
		HINST_THISCOMPONENT,		// hInstance
		nullptr,			// hIcon
		nullptr,			// hCursor
		nullptr,			// hbrBackground
		nullptr,			// lpszMenuName
		WC_MESSAGEWIDGET,		// lpszClassName
		nullptr				// hIconSm
	};

	atom_messageWidget = RegisterClassEx(&wndClass);
}

void MessageWidgetUnregister(void)
{
	if (atom_messageWidget != 0) {
		UnregisterClass(MAKEINTATOM(atom_messageWidget), HINST_THISCOMPONENT);
		atom_messageWidget = 0;
	}
}
