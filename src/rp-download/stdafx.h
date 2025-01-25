/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * stdafx.h: Common definitions and includes for COM.                      *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifdef _WIN32
// Windows SDK defines and includes.
#include "libwin32common/RpWin32_sdk.h"

// Additional Windows headers.
#include <shlobj.h>
#include <tchar.h>
#endif /* _WIN32 */

#ifdef __cplusplus
/** C++ **/

// C includes (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>

// C++ includes
#include <algorithm>
#include <string>

// libfmt
#include "rp-libfmt.h"

#else /* !__cplusplus */
/** C **/

// C includes
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#endif /* __cplusplus */

#include "common.h"
#include "ctypex.h"
#include "tcharx.h"
