/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * exe_res_structs.h: DOS/Windows executable structures. (resources)       *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Based on w32api's winnt.h.
// References:
// - https://github.com/MaxKellermann/w32api/blob/440c229960e782831d01c6638661f1c40cadbeb5/include/winnt.h
// - https://github.com/MaxKellermann/w32api/blob/440c229960e782831d01c6638661f1c40cadbeb5/include/winver.h
// - http://www.brokenthorn.com/Resources/OSDevPE.html
// - https://docs.microsoft.com/en-us/windows/win32/menurc/resource-types
// - http://sandsprite.com/CodeStuff/Understanding_imports.html
// - https://docs.microsoft.com/en-us/windows/win32/debug/pe-format

// Resource types
typedef enum {
	RT_CURSOR	= 1,
	RT_BITMAP	= 2,
	RT_ICON		= 3,
	RT_MENU		= 4,
	RT_DIALOG	= 5,
	RT_STRING	= 6,
	RT_FONTDIR	= 7,
	RT_FONT		= 8,
	RT_ACCELERATOR	= 9,
	RT_RCDATA	= 10,
	RT_MESSAGETABLE	= 11,
	RT_GROUP_CURSOR	= 12,	// (RT_CURSOR+11)
	RT_GROUP_ICON	= 14,	// (RT_ICON+11)
	RT_VERSION	= 16,
	RT_DLGINCLUDE	= 17,
	RT_PLUGPLAY	= 19,
	RT_VXD		= 20,
	RT_ANICURSOR	= 21,
	RT_ANIICON	= 22,
	RT_HTML		= 23,
	RT_MANIFEST	= 24,

	// MFC resources
	RT_DLGINIT	= 240,
	RT_TOOLBAR	= 241,
} ResourceType;

/** Version resource **/

//#define VS_FILE_INFO RT_VERSION	// TODO
#define VS_VERSION_INFO 1
#define VS_USER_DEFINED 100
#define VS_FFI_SIGNATURE 0xFEEF04BD
#define VS_FFI_STRUCVERSION 0x10000
#define VS_FFI_FILEFLAGSMASK 0x3F
typedef enum {
	VS_FF_DEBUG		= 0x01,
	VS_FF_PRERELEASE	= 0x02,
	VS_FF_PATCHED		= 0x04,
	VS_FF_PRIVATEBUILD	= 0x08,
	VS_FF_INFOINFERRED	= 0x10,
	VS_FF_SPECIALBUILD	= 0x20,
} VS_FileFlags;

// updated from: https://source.winehq.org/git/wine.git/blob/7d77d330a5b60be918dbf17d9d9ca357d93bff29:/include/verrsrc.h
typedef enum {
	VOS_UNKNOWN		= 0,

	VOS_DOS			= 0x10000,
	VOS_OS216		= 0x20000,
	VOS_OS232		= 0x30000,
	VOS_NT			= 0x40000,
	VOS_WINCE		= 0x50000,

	VOS__BASE		= 0,
	VOS__WINDOWS16		= 1,
	VOS__PM16		= 2,
	VOS__PM32		= 3,
	VOS__WINDOWS32		= 4,

	VOS_DOS_WINDOWS16	= (VOS_DOS | VOS__WINDOWS16),
	VOS_DOS_WINDOWS32	= (VOS_DOS | VOS__WINDOWS32),
	VOS_OS216_PM16		= (VOS_OS216 | VOS__PM16),
	VOS_OS232_PM32		= (VOS_OS232 | VOS__PM32),
	VOS_NT_WINDOWS32	= (VOS_NT | VOS__WINDOWS32),
} VS_OperatingSystem;

typedef enum {
	VFT_UNKNOWN = 0,
	VFT_APP = 1,
	VFT_DLL = 2,
	VFT_DRV = 3,
	VFT_FONT = 4,
	VFT_VXD = 5,
	VFT_STATIC_LIB = 7,
} VS_FileType;

typedef enum {
	VFT2_UNKNOWN = 0,
	VFT2_DRV_PRINTER = 1,
	VFT2_DRV_KEYBOARD = 2,
	VFT2_DRV_LANGUAGE = 3,
	VFT2_DRV_DISPLAY = 4,
	VFT2_DRV_MOUSE = 5,
	VFT2_DRV_NETWORK = 6,
	VFT2_DRV_SYSTEM = 7,
	VFT2_DRV_INSTALLABLE = 8,
	VFT2_DRV_SOUND = 9,
	VFT2_DRV_COMM = 10,
	VFT2_DRV_INPUTMETHOD = 11,
	VFT2_DRV_VERSIONED_PRINTER = 12,
	VFT2_FONT_RASTER = 1,
	VFT2_FONT_VECTOR = 2,
	VFT2_FONT_TRUETYPE = 3,
} VS_FileSubtype;

/**
 * Version info resource (fixed-size data section)
 */
typedef struct _VS_FIXEDFILEINFO {
	uint32_t dwSignature;
	uint32_t dwStrucVersion;
	uint32_t dwFileVersionMS;
	uint32_t dwFileVersionLS;
	uint32_t dwProductVersionMS;
	uint32_t dwProductVersionLS;
	uint32_t dwFileFlagsMask;
	uint32_t dwFileFlags;
	uint32_t dwFileOS;
	uint32_t dwFileType;
	uint32_t dwFileSubtype;
	uint32_t dwFileDateMS;
	uint32_t dwFileDateLS;
} VS_FIXEDFILEINFO;
ASSERT_STRUCT(VS_FIXEDFILEINFO, 13*sizeof(uint32_t));

#ifdef __cplusplus
}
#endif
