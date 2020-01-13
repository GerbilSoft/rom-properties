/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * stdafx.h: Common definitions and includes for COM.                      *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_STDAFX_H__
#define __ROMPROPERTIES_WIN32_STDAFX_H__

// Make sure STRICT is defined for better type safety.
#ifndef STRICT
#define STRICT
#endif

// Windows SDK defines and includes.
#include "libwin32common/RpWin32_sdk.h"
#include <windows.h>

#if _WIN32_WINNT < 0x0600
# error Windows Vista SDK or later is required.
#endif

// Typesafe inline function wrappers for some Windows headers.
#include "libwin32common/sdk/windowsx_ts.h"
#include "libwin32common/sdk/commctrl_ts.h"

// Subclass functions for compatibility with older Windows.
#include "libwin32common/SubclassWindow.h"

// Additional Windows headers.
#include <olectl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlwapi.h>
#include <commdlg.h>

// Native COM support. (C++ only!)
#ifdef __cplusplus
# include <comdef.h>
#endif

// IEmptyVolumeCache and related.
#include <emptyvc.h>

// Common Controls COM declarations.
#include <commoncontrols.h>

/** C/C++ headers **/

#ifdef __cplusplus
/** C++ **/

// C includes. (C++ namespace)
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include <stdint.h>
/*
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <ctime>
*/

// C++ includes.
#include <algorithm>
#include <array>
#include <list>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#else /* !__cplusplus */
/** C **/

// C includes.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_WIN32_STDAFX_H__ */
