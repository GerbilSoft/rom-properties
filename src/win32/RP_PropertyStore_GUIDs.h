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
DEFINE_PROPERTYKEY(PKEY_RomProperties_GameID,		0xb608e986, 0xb892, 0x422a, 0x9b, 0xe9, 0x7c, 0x3d, 0x44, 0x27, 0xe7, 0x8f, 2);
DEFINE_PROPERTYKEY(PKEY_RomProperties_TitleID,		0x0217a595, 0xc88b, 0x41bf, 0xa5, 0x8c, 0x5d, 0xa4, 0x48, 0x3d, 0xee, 0xe2, 2);
DEFINE_PROPERTYKEY(PKEY_RomProperties_MediaID,		0x53e92dab, 0x99e2, 0x4897, 0xb7, 0x8b, 0xfe, 0xe5, 0x98, 0x32, 0xa3, 0x23, 2);
DEFINE_PROPERTYKEY(PKEY_RomProperties_OSVersion,	0x655ca042, 0xff37, 0x4564, 0x8a, 0xc9, 0x32, 0x1f, 0x9c, 0x5f, 0x2a, 0x8d, 2);
DEFINE_PROPERTYKEY(PKEY_RomProperties_EncryptionKey,	0xf31225fa, 0xb303, 0x454e, 0x83, 0x89, 0x69, 0xc1, 0xb8, 0xca, 0x38, 0xc6, 2);