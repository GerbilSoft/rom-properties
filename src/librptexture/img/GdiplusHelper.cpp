/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * GdiplusHelper.cpp: GDI+ helper class. (Win32)                           *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GdiplusHelper.hpp"

// Gdiplus for PNG decoding.
// NOTE: Gdiplus requires min/max.
#include <algorithm>
namespace Gdiplus {
	using std::min;
	using std::max;
}
#include <olectl.h>
#include <gdiplus.h>

/**
 * Initialize GDI+.
 * @return GDI+ token, or 0 on failure.
 */
ULONG_PTR GdiplusHelper::InitGDIPlus(void)
{
	Gdiplus::GdiplusStartupInput gdipSI;
	gdipSI.GdiplusVersion = 1;
	gdipSI.DebugEventCallback = nullptr;
	gdipSI.SuppressBackgroundThread = FALSE;
	gdipSI.SuppressExternalCodecs = FALSE;
	ULONG_PTR gdipToken;
	Gdiplus::Status status = GdiplusStartup(&gdipToken, &gdipSI, nullptr);
	return (status == Gdiplus::Status::Ok ? gdipToken : 0);
}

/**
 * Shut down GDI+.
 * @param gdipToken GDI+ token.
 */
void GdiplusHelper::ShutdownGDIPlus(ULONG_PTR gdipToken)
{
	Gdiplus::GdiplusShutdown(gdipToken);
}
