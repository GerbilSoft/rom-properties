/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * resource.rc: Win32 resource script.                                     *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

/** Icons **/
#define IDI_KEY_VALID				201

/** PNG images **/
#define RT_PNG					0x4E50	/* 'PN', byteswapped */
#define IDP_FLAGS_16x16				301
#define IDP_FLAGS_24x24				302
#define IDP_FLAGS_32x32				303
#define IDP_ACH_16x16				311
#define IDP_ACH_24x24				312
#define IDP_ACH_32x32				313
#define IDP_ACH_64x64				314
#define IDP_ACH_GRAY_16x16			321
#define IDP_ACH_GRAY_24x24			322
#define IDP_ACH_GRAY_32x32			323
#define IDP_ACH_GRAY_64x64			324

// Menus
#define IDR_ECKS_BAWKS				38727
#define IDM_ECKS_BAWKS_1			38728
#define IDM_ECKS_BAWKS_2			38729

// Dialogs
#define IDD_PROPERTY_SHEET			100	// Generic property sheet
#define IDD_SUBTAB_CHILD_DIALOG			101	// Subtab child dialog
#define IDD_XATTRVIEW				102	// Extended Attribute viewer

// Standard controls
#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

// Property sheet controls.
// These are defined in Wine's comctl32.h, but not the Windows SDK.
// Reference: https://source.winehq.org/git/wine.git/blob/HEAD:/dlls/comctl32/comctl32.h
#define IDC_TABCONTROL				12320
#define IDC_APPLY_BUTTON			12321
#define IDC_BACK_BUTTON				12323
#define IDC_NEXT_BUTTON				12324
#define IDC_FINISH_BUTTON			12325
#define IDC_SUNKEN_LINE				12326
#define IDC_SUNKEN_LINEHEADER			12327

// Custom property sheet controls.
#define IDC_RP_RESET				13431
#define IDC_RP_DEFAULTS				13432
#define IDC_RP_OPTIONS				13433

// "Reset" and "Defaults" messages.
// The parent PropertySheet doesn't store any user data, so we can't
// store a reference to the private class. Hence, we have to send a
// message to all child property sheet pages instead.
#define WM_RP_PROP_SHEET_RESET			(WM_USER + 0x1234)
#define WM_RP_PROP_SHEET_DEFAULTS		(WM_USER + 0x1235)
// Enable/disable the "Defaults" button.
// We can't access the ITab objects from the PropertySheet callback function.
// wParam: 0 == disable, 1 == enable
#define WM_RP_PROP_SHEET_ENABLE_DEFAULTS	(WM_USER + 0x1236)
#define RpPropSheet_EnableDefaults(hWnd,enable)	(void)SNDMSG(hWnd,WM_RP_PROP_SHEET_ENABLE_DEFAULTS,(WPARAM)(enable),0)

// KeyStoreWin32 messages.
// Basically the equivalent of Qt's signals.
// NOTE: We could pack sectIdx/keyIdx into WPARAM,
// but that would change the way signals are handled.
// Define two messages for consistency.

// wParam: sectIdx
// lParam: keyIdx
#define WM_KEYSTORE_KEYCHANGED_SECTKEY				(WM_USER + 0x2001)
#define KeyStore_KeyChanged_SectKey(hWnd,sectIdx,keyIdx)	(void)SNDMSG(hWnd,WM_KEYSTORE_KEYCHANGED_SECTKEY,(sectIdx),(keyIdx))

// wParam: 0
// lParam: idx
#define WM_KEYSTORE_KEYCHANGED_IDX				(WM_USER + 0x2002)
#define KeyStore_KeyChanged_Idx(hWnd,idx)			(void)SNDMSG(hWnd,WM_KEYSTORE_KEYCHANGED_IDX,0,(idx))

// wParam: 0
// lParam: 0
#define WM_KEYSTORE_ALLKEYSCHANGED				(WM_USER + 0x2003)
#define KeyStore_AllKeysChanged_Idx(hWnd)			(void)SNDMSG(hWnd,WM_KEYSTORE_ALLKEYSCHANGED,0,0)

// wParam: 0
// lParam: 0
#define WM_KEYSTORE_MODIFIED					(WM_USER + 0x2004)
#define KeyStore_Modified(hWnd)					(void)SNDMSG(hWnd,WM_KEYSTORE_MODIFIED,0,0)

/**** rp-config ****/

/** Configuration dialog **/
#define IDD_CONFIG_IMAGETYPES			110
#define IDD_CONFIG_SYSTEMS			111
#define IDD_CONFIG_OPTIONS			112
#define IDD_CONFIG_CACHE			113
#define IDD_CONFIG_CACHE_XP			114
#define IDD_CONFIG_ACHIEVEMENTS			115
#define IDD_CONFIG_KEYMANAGER			116
#define IDD_CONFIG_ABOUT			117

// Image type priorities
#define IDC_IMAGETYPES_DESC1			40001
#define IDC_IMAGETYPES_DESC2			40002
#define IDC_IMAGETYPES_CREDITS			40003
#define IDC_IMAGETYPES_VIEWPORT_OUTER		40010
#define IDC_IMAGETYPES_VIEWPORT_INNER		40011

// Systems
#define IDC_SYSTEMS_DMGTS_GROUPBOX		40101
#define IDC_SYSTEMS_DMGTS_DMG			40102
#define IDC_SYSTEMS_DMGTS_SGB			40103
#define IDC_SYSTEMS_DMGTS_CGB			40104

// Options
#define IDC_OPTIONS_GRPDOWNLOADS		40201
#define IDC_OPTIONS_GRPEXTIMGDL			40202
#define IDC_OPTIONS_CHKEXTIMGDL			40203
#define IDC_OPTIONS_LBL_UNMETERED_DL		40204
#define IDC_OPTIONS_CBO_UNMETERED_DL		40205
#define IDC_OPTIONS_LBL_METERED_DL		40206
#define IDC_OPTIONS_CBO_METERED_DL		40207
#define IDC_OPTIONS_INTICONSMALL		40208
#define IDC_OPTIONS_STOREFILEORIGININFO		40209
#define IDC_OPTIONS_PALLANGUAGEFORGAMETDB	40210
#define IDC_OPTIONS_GRPOPTIONS			40211
#define IDC_OPTIONS_DANGEROUSPERMISSIONS	40212
#define IDC_OPTIONS_ENABLETHUMBNAILONNETWORKFS	40213
#define IDC_OPTIONS_THUMBNAILDIRECTORYPACKAGES	40214
#define IDC_OPTIONS_SHOWXATTRVIEW		40215

// Thumbnail cache
#define IDC_CACHE_DESCRIPTION			40301
#define IDC_CACHE_CLEAR_SYS_THUMBS		40302
#define IDC_CACHE_CLEAR_RP_DL			40303
#define IDC_CACHE_STATUS			40304
#define IDC_CACHE_PROGRESS			40305
#define IDC_CACHE_XP_FIND_DRIVES		40306
#define IDC_CACHE_XP_FIND_PATH			40307
#define IDC_CACHE_XP_DRIVES			40308
#define IDC_CACHE_XP_PATH			40309
#define IDC_CACHE_XP_BROWSE			40310
#define IDC_CACHE_XP_CLEAR_SYS_THUMBS		40311

// Achievements
#define IDC_ACHIEVEMENTS_LIST			40401

// FIXME: Only if ENABLE_DECRYPTION, but we can't include config.librpbase.h.
#if 1
// Key Manager
#define IDC_KEYMANAGER_LIST			40501
#define IDC_KEYMANAGER_EDIT			40502
#define IDC_KEYMANAGER_IMPORT			40503

// Key Manager: "Import" menu
//#define IDR_KEYMANAGER_IMPORT			30501
#define IDM_KEYMANAGER_IMPORT_WII_KEYS_BIN	30502
#define IDM_KEYMANAGER_IMPORT_WIIU_OTP_BIN	30503
#define IDM_KEYMANAGER_IMPORT_3DS_BOOT9_BIN	30504
#define IDM_KEYMANAGER_IMPORT_3DS_AESKEYDB	30505
#endif

// About
#define IDC_ABOUT_ICON				40501
#define IDC_ABOUT_LINE1				40502
#define IDC_ABOUT_LINE2				40503
#define IDC_ABOUT_VERSION			40504
#define IDC_ABOUT_UPDATE_CHECK			40505
#define IDC_ABOUT_TABCONTROL			40506
#define IDC_ABOUT_RICHEDIT			40507

/**** RP_XAttrView ****/

#define IDC_XATTRVIEW_DOS_READONLY		42001
#define IDC_XATTRVIEW_DOS_HIDDEN		42002
#define IDC_XATTRVIEW_DOS_ARCHIVE		42003
#define IDC_XATTRVIEW_DOS_SYSTEM		42004
#define IDC_XATTRVIEW_NTFS_COMPRESSED		42005
#define IDC_XATTRVIEW_NTFS_ENCRYPTED		42006
#define IDC_XATTRVIEW_GRPADS			42007
#define IDC_XATTRVIEW_LISTVIEW_ADS		42008
