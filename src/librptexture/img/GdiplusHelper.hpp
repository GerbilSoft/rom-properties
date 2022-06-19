/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * GdiplusHelper.hpp: GDI+ helper class. (Win32)                           *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_IMG_GDIPLUSHELPER_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IMG_GDIPLUSHELPER_HPP__

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
ULONG_PTR InitGDIPlus(void);

/**
 * Shut down GDI+.
 * @param gdipToken GDI+ token.
 */
void ShutdownGDIPlus(ULONG_PTR gdipToken);

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IMG_GDIPLUSHELPER_HPP__ */
