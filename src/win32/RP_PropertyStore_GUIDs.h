/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_PropertyStore_GUIDs.c: Custom property GUID definitions.             *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "libwin32common/RpWin32_sdk.h"

// Windows: Property keys and variants
#include <objidl.h>
#include <propkey.h>
#include <propvarutil.h>

// Custom properties
DEFINE_PROPERTYKEY(PKEY_RomProperties_GameID, 0xb608e986, 0xb892, 0x422a, 0x9b, 0xe9, 0x7c, 0x3d, 0x44, 0x27, 0xe7, 0x8f, 2);
DEFINE_PROPERTYKEY(PKEY_RomProperties_SerialNumber, 0x4a3dad67, 0x24f1, 0x4478, 0x8e, 0x27, 0x04, 0x07, 0x18, 0x60, 0x37, 0x84, 2);
DEFINE_PROPERTYKEY(PKEY_RomProperties_IOSVersion, 0x655ca042, 0xff37, 0x4564, 0x8a, 0xc9, 0x32, 0x1f, 0x9c, 0x5f, 0x2a, 0x8d, 2);
