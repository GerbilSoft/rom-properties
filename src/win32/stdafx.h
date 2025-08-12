/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * stdafx.h: Common definitions and includes for COM.                      *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Make sure STRICT is defined for better type safety.
#ifndef STRICT
#define STRICT
#endif

// Windows SDK defines and includes.
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/rp_versionhelpers.h"
#include <windows.h>

#if _WIN32_WINNT < 0x0600
#  error Windows Vista SDK or later is required.
#endif

// Typesafe inline function wrappers for some Windows headers.
#include "libwin32common/sdk/windowsx_ts.h"
#include "libwin32common/sdk/commctrl_ts.h"

// Additional Windows headers
#include <olectl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <objidl.h>

// FIXME: shlobj.h on MinGW-w64 on AppVeyor doesn't properly inline a few
// functions when building in C mode, resulting in multiple definition errors.
// C:/mingw-w64/x86_64-7.2.0-posix-seh-rt_v5-rev1
// - FreeIDListArray
// - FreeKnownFolderDefinitionFields
// - IDListContainerIsConsistent
#if defined(_MSC_VER) || defined(__cplusplus)
# include <shlobj.h>
#endif /* _MSC_VER || __cplusplus */

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
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <array>
#include <forward_list>
#include <locale>
#include <memory>
#include <numeric>
#include <set>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// libfmt
#ifndef RP_NO_INCLUDE_LIBFMT_IN_STDAFX_H
#  include "rp-libfmt.h"
#endif

#else /* !__cplusplus */
/** C **/

// C includes.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#endif /* __cplusplus */

// libwin32common C headers
#include "libwin32common/w32time.h"
// libwin32ui C headers
#include "libwin32ui/HiDPI.h"

#ifdef __cplusplus
// libwin32common C++ headers
#  include "libwin32common/ComBase.hpp"
// libwin32ui C++ headers
#  include "libwin32ui/RegKey.hpp"
#  include "libwin32ui/WinUI.hpp"
#  include "libwin32ui/WTSSessionNotification.hpp"
#endif /* __cplusplus */

// libi18n
#include "libi18n/i18n.hpp"

// librpbase common headers
#include "common.h"
#include "aligned_malloc.h"
#include "ctypex.h"
#include "dll-macros.h"

#ifdef __cplusplus
// librpbase C++ headers
#include "librpbase/RomData.hpp"
#include "librpbase/config/Config.hpp"

// librpfile C++ headers
#include "librpfile/IRpFile.hpp"
#include "librpfile/RpFile.hpp"
#include "librpfile/FileSystem.hpp"

// librptexture C++ headers
#include "librptexture/img/rp_image.hpp"

// libromdata C++ headers
#include "libromdata/RomDataFactory.hpp"

// librptext C++ headers
#include "librptext/conversion.hpp"
#include "librptext/wchar.hpp"
#endif /* __cplusplus */
