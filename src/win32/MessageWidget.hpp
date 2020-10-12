/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * MessageWidget.hpp: Message widget. (Similar to KMessageWidget)          *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_MESSAGEWIDGET_HPP__
#define __ROMPROPERTIES_WIN32_MESSAGEWIDGET_HPP__

#include "libwin32common/RpWin32_sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WC_MESSAGEWIDGET		_T("rp-MessageWidget")

void MessageWidgetRegister(void);
void MessageWidgetUnregister(void);

#define WM_MSGW_SET_MESSAGE_TYPE	(WM_USER + 1)	// wParam == messageType
#define WM_MSGW_GET_MESSAGE_TYPE	(WM_USER + 2)	// return == messageType

#define MSGWN_FIRST			(NM_LAST - 2600U)
#define MSGWN_CLOSED			(MSGWN_FIRST - 1)

static inline void MessageWidget_SetMessageType(HWND hWnd, int messageType)
{
	SendMessage(hWnd, WM_MSGW_SET_MESSAGE_TYPE, static_cast<WPARAM>(messageType), 0);
}

static inline int MessageWidget_GetMessageType(HWND hWnd)
{
	return static_cast<int>(SendMessage(hWnd, WM_MSGW_GET_MESSAGE_TYPE, 0, 0));
}

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_WIN32_DRAGIMAGELABEL_HPP__ */
