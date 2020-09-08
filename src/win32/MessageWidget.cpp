/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * MessageWidget.cpp: Message widget. (Similar to KMessageWidget)          *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
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
		MessageWidgetPrivate(HWND hWnd)
			: hWnd(hWnd)
			, hFont(nullptr)
			, hUser32(GetModuleHandle(_T("user32")))
			, hIcon(nullptr)
			, hBtnClose(nullptr)
			, messageType(MB_ICONINFORMATION)
		{
			assert(hUser32 != nullptr);

			// TODO: Update szIcon on system DPI change.
			szIcon.cx = GetSystemMetrics(SM_CXSMICON);
			szIcon.cy = GetSystemMetrics(SM_CYSMICON);

			// Initialize the icon.
			updateIcon();

			// Create the Close button.
			// TODO: Icon.
			// TODO: Reposition on WM_SIZE?
			// FIXME: The button isn't working...
			RECT rect;
			GetClientRect(hWnd, &rect);

			POINT ptBtnClose; SIZE szBtnClose;
			szBtnClose.cx = szIcon.cx + BORDER_SIZE;
			szBtnClose.cy = szIcon.cy + BORDER_SIZE;
			ptBtnClose.x = rect.right - szBtnClose.cx - BORDER_SIZE;
			ptBtnClose.y = (rect.bottom - szBtnClose.cy) / 2;
			hBtnClose = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
				WC_BUTTON, nullptr,
				WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_PUSHBUTTON,
				ptBtnClose.x, ptBtnClose.y,
				szBtnClose.cx, szBtnClose.cy,
				hWnd, (HMENU)IDC_CLOSE_BUTTON,
				nullptr, nullptr);
		}

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
		void updateIcon(void)
		{
			LPCTSTR lpszRes;
			switch (messageType) {
				case MB_ICONEXCLAMATION:
					lpszRes = MAKEINTRESOURCE(101);
					break;
				case MB_ICONQUESTION:
					lpszRes = MAKEINTRESOURCE(102);
					break;
				case MB_ICONSTOP:
					lpszRes = MAKEINTRESOURCE(103);
					break;
				case MB_ICONINFORMATION:
				default:
					lpszRes = MAKEINTRESOURCE(104);
					break;
			}
			hIcon = (HICON)LoadImage(hUser32, lpszRes, IMAGE_ICON,
				szIcon.cx, szIcon.cy, LR_SHARED);

			// Invalidate the icon portion.
			RECT rectIcon = {4, 4, szIcon.cx, szIcon.cy};
			InvalidateRect(hWnd, &rectIcon, TRUE);
		}

	public:
		/** Window Message functions **/

		/**
		 * WM_PAINT handler.
		 */
		void paint(void);

	public:
		HWND hWnd;			// MessageWidget control
		HFONT hFont;			// set by the parent window
		HMODULE hUser32;		// USER32.DLL
		HICON hIcon;			// loaded with LR_SHARED
		HWND hBtnClose;			// initialized in WM_CREATE

		unsigned int messageType;	// MB_ICON*
		SIZE szIcon;			// Icon size
};

/**
 * WM_PAINT handler.
 */
void MessageWidgetPrivate::paint(void)
{
	RECT rect;
	GetClientRect(hWnd, &rect);

	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hWnd, &ps);
	SelectObject(hDC, hFont);
	SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
	SetBkMode(hDC, OPAQUE);

	// Clear the background so we don't end up drawing over
	// the previous icon/text.
	FillRect(hDC, &rect, nullptr);

	if (hIcon) {
		DrawIconEx(hDC, BORDER_SIZE, BORDER_SIZE, hIcon, szIcon.cx, szIcon.cy, 0, nullptr, DI_NORMAL);
		rect.left += szIcon.cx + BORDER_SIZE*2;
	} else {
		rect.left += BORDER_SIZE;
	}

	TCHAR tbuf[128];
	int len = GetWindowTextLength(hWnd);
	if (len < 128) {
		GetWindowText(hWnd, tbuf, _countof(tbuf));
		DrawText(hDC, tbuf, len, &rect, DT_SINGLELINE | DT_VCENTER);
	} else {
		TCHAR *const tmbuf = static_cast<TCHAR*>(malloc((len+1) * sizeof(TCHAR)));
		GetWindowText(hWnd, tmbuf, len+1);
		DrawText(hDC, tmbuf, len, &rect, DT_SINGLELINE | DT_VCENTER);
		free(tmbuf);
	}
	EndPaint(hWnd, &ps);
}

static LRESULT CALLBACK
MessageWidgetWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_CREATE: {
			MessageWidgetPrivate *const d = new MessageWidgetPrivate(hWnd);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(d));
			break;
		}

		case WM_DESTROY: {
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

		case WM_PAINT: {
			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			d->paint();
			return 0;
		}

		case WM_SETFONT: {
			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			d->hFont = reinterpret_cast<HFONT>(wParam);
			if (LOWORD(lParam)) {
				InvalidateRect(hWnd, nullptr, TRUE);
			}
			return 0;
		}

		case WM_GETFONT: {
			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			return reinterpret_cast<LRESULT>(d->hFont);
		}

		case WM_MSGW_SET_MESSAGE_TYPE: {
			MessageWidgetPrivate *const d = reinterpret_cast<MessageWidgetPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			d->setMessageType(static_cast<unsigned int>(wParam));
			return 0;
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

	static const WNDCLASS wndClass = {
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
	};

	atom_messageWidget = RegisterClass(&wndClass);
}

void MessageWidgetUnregister(void)
{
	if (atom_messageWidget != 0) {
		UnregisterClass(MAKEINTRESOURCE(atom_messageWidget), HINST_THISCOMPONENT);
	}
}
