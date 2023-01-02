/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * GdiplusHelper.hpp: GDI+ helper class. (Win32)                           *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifndef _WIN32
#error GdiplusHelper is Win32 only.
#endif

#include "common.h"
#include "libwin32common/RpWin32_sdk.h"

namespace GdiplusHelper {

/**
 * Initialize GDI+.
 * @return GDI+ token, or 0 on failure.
 */
RP_LIBROMDATA_PUBLIC
ULONG_PTR InitGDIPlus(void);

/**
 * Shut down GDI+.
 * @param gdipToken GDI+ token.
 */
RP_LIBROMDATA_PUBLIC
void ShutdownGDIPlus(ULONG_PTR gdipToken);

}
