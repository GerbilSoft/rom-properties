/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * OptionsMenuButton_decl.h: Common declarationss for OptionsMenuButton.   *
 * NOTE: Should only be included by the OptionsMenuButton header file.     *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum StandardOptionID {
	OPTION_EXPORT_TEXT = -1,
	OPTION_EXPORT_JSON = -2,
	OPTION_COPY_TEXT = -3,
	OPTION_COPY_JSON = -4,
};

#ifdef __cplusplus
}
#endif
