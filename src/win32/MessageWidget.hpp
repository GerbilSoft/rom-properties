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

#define WC_MESSAGEWIDGET _T("rp-MessageWidget")

#define WM_MSGW_SET_MESSAGE_TYPE	(WM_USER + 1)
#define WM_MSGW_GET_MESSAGE_TYPE	(WM_USER + 2)

void MessageWidgetRegister(void);
void MessageWidgetUnregister(void);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_WIN32_DRAGIMAGELABEL_HPP__ */
