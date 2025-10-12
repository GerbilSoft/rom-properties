/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_PrivateExtractIcons.cpp: PrivateExtractIcons() implementation.       *
 *                                                                         *
 * Copyright (c) 2025 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "libwin32common/RpWin32_sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DllMain() DLL_PROCESS_ATTACH function.
 * @return Win32 error code from the Detours library.
 */
LONG RP_PrivateExtractIcons_DllProcessAttach(void);

/**
 * DllMain() DLL_PROCESS_DETACH function.
 * @return Win32 error code from the Detours library.
 */
LONG RP_PrivateExtractIcons_DllProcessDetach(void);

#ifdef __cplusplus
}
#endif
