/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * com-iids.c: COM IIDs for interfaces not supported by MinGW-w64.         *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.win32.h"

#ifndef HAVE_THUMBCACHE_H
DEFINE_GUID(IID_IThumbnailProvider, 0xe357fccd, 0xa995, 0x4576, 0xb0,0x1f, 0x23, 0x46, 0x30, 0x15, 0x4e, 0x96);
#endif /* HAVE_THUMBCACHE_H */
