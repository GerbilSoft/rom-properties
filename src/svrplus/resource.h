/***************************************************************************
 * ROM Properties Page shell extension installer. (svrplus)                *
 * resource.h: Win32 resource script.                                      *
 *                                                                         *
 * Copyright (c) 2017 by Egor.                                             *
 * Copyright (c) 2017-2028 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// svrplus dialog, icon, and controls
#define IDD_SVRPLUS                             101
#define IDI_SVRPLUS                             102
#define IDC_STATIC_DESC                         1001
#define IDC_STATIC_ICON                         1002
#define IDC_STATIC_STATUS1                      1003
#define IDC_STATIC_STATUS2                      1004
#define IDC_BUTTON_INSTALL                      1005
#define IDC_BUTTON_UNINSTALL                    1006

// svrplus string table IDs (IDS_* == plain string without formatting)
#define IDS_MAIN_DESCRIPTION                    2001
#define IDS_ISR_FATAL_ERROR                     2002
#define IDS_ISR_PROCESS_STILL_ACTIVE            2003
#define IDS_ISR_REGSVR32_FAIL_ARGS              2004
#define IDS_ISR_REGSVR32_FAIL_OLE               2005
#define IDS_DLLS_REGISTERING                    2006
#define IDS_DLL_REGISTERING                     2007
#define IDS_DLLS_UNREGISTERING                  2008
#define IDS_DLL_UNREGISTERING                   2009
#define IDS_DLLS_REG_OK                         2010
#define IDS_DLL_REG_OK                          2011
#define IDS_DLLS_UNREG_OK                       2012
#define IDS_DLL_UNREG_OK                        2013
#define IDS_DLLS_REG_ERROR                      2014
#define IDS_DLL_REG_ERROR                       2015
#define IDS_DLLS_UNREG_ERROR                    2016
#define IDS_DLL_UNREG_ERROR                     2017
#define IDS_ERROR_STARTING_WORKER_THREAD        2018
#define IDS_ERROR_COULD_NOT_OPEN_URL_TITLE      2019

// svrplus string table IDs (IDSF_* == has C format strings)
#define IDSF_MSVCRT_DOWNLOAD_AT                 3001
#define IDSF_MSVCRT_MISSING_ONE                 3002
#define IDSF_MSVCRT_MISSING_MULTIPLE            3003
#define IDSF_ISR_INVALID_ARCH                   3004
#define IDSF_ISR_FILE_NOT_FOUND                 3005
#define IDSF_ISR_CREATEPROCESS_FAILED           3006
#define IDSF_ISR_REGSVR32_FAIL_LOAD             3007
#define IDSF_ISR_REGSVR32_FAIL_ENTRY            3008
#define IDSF_ISR_REGSVR32_FAIL_REG              3009
#define IDSF_ISR_REGSVR32_UNKNOWN_EXIT_CODE     3010
#define IDSF_ERROR_COULD_NOT_OPEN_URL           3011
