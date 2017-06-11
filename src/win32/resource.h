/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * resource.rc: Win32 resource script.                                     *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_RESOURCE_H__
#define __ROMPROPERTIES_WIN32_RESOURCE_H__

#include "config.librpbase.h"

// Dialogs
#define IDD_PROPERTY_SHEET                      100	/* Generic property sheet. */
#define IDD_SUBTAB_CHILD_DIALOG                 101	/* Subtab child dialog. */

// Standard controls
#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

// Property sheet controls.
// These are defined in Wine's comctl32.h, but not the Windows SDK.
// Reference: https://source.winehq.org/git/wine.git/blob/HEAD:/dlls/comctl32/comctl32.h
#define IDC_TABCONTROL                          12320
#define IDC_APPLY_BUTTON                        12321
#define IDC_BACK_BUTTON                         12323
#define IDC_NEXT_BUTTON                         12324
#define IDC_FINISH_BUTTON                       12325
#define IDC_SUNKEN_LINE                         12326
#define IDC_SUNKEN_LINEHEADER                   12327

// Custom property sheet controls.
#define IDC_RP_RESET                            13431
#define IDC_RP_DEFAULTS                         13432

// "Reset" and "Defaults" messages.
// The parent PropertySheet doesn't store any user data, so we can't
// store a reference to the private class. Hence, we have to send a
// message to all child property sheet pages instead.
#define WM_RP_PROP_SHEET_RESET			(WM_USER + 0x1234)
#define WM_RP_PROP_SHEET_DEFAULTS		(WM_USER + 0x1235)

/** Configuration dialog **/
#define IDD_CONFIG_IMAGETYPES                   110
#define IDD_CONFIG_DOWNLOADS                    111
#define IDD_CONFIG_CACHE                        112
#define IDD_CONFIG_CACHE_XP                     113
#define IDD_CONFIG_KEYMANAGER                   114

// Image type priorities.
#define IDC_IMAGETYPES_DESC1                    40001
#define IDC_IMAGETYPES_DESC2                    40002
#define IDC_IMAGETYPES_CREDITS                  40003

// Downloads
#define IDC_EXTIMGDL                            40101
#define IDC_INTICONSMALL                        40102
#define IDC_HIGHRESDL                           40103

// Thumbnail cache
#define IDC_CACHE_CLEAR_SYS_THUMBS              40201
#define IDC_CACHE_CLEAR_RP_DL                   40202
#define IDC_CACHE_STATUS                        40203
#define IDC_CACHE_PROGRESS                      40204
#define IDC_CACHE_XP_FIND_DRIVES                40205
#define IDC_CACHE_XP_FIND_PATH                  40206
#define IDC_CACHE_XP_DRIVES                     40207
#define IDC_CACHE_XP_PATH                       40208
#define IDC_CACHE_XP_BROWSE                     40209
#define IDC_CACHE_XP_CLEAR_SYS_THUMBS           40210

#ifdef ENABLE_DECRYPTION
// Key Manager
#define IDC_KEYMANAGER_LIST                     40301
#define IDC_KEYMANAGER_IMPORT                   40302

// Key Manager: "Import" menu
#define IDR_KEYMANAGER_IMPORT                   30301
#define IDC_KEYMANAGER_IMPORT_WII_KEYS_BIN      30302
#define IDC_KEYMANAGER_IMPORT_3DS_BOOT9_BIN     30303
#define IDC_KEYMANAGER_IMPORT_3DS_AESKEYDB      30304
#endif /* ENABLE_DECRYPTION */

#endif /* __ROMPROPERTIES_WIN32_RESOURCE_H__ */
