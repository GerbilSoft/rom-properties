/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * stdafx.h: Common definitions and includes.                              *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPFILE_STDAFX_H__
#define __ROMPROPERTIES_LIBRPFILE_STDAFX_H__

#ifdef __cplusplus
/** C++ **/

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>
#include <ctime>
#include <stdint.h>
#if 0
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <stdlib.h>
#endif

// C++ includes.
#include <algorithm>
#if 0
#include <array>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#endif

#else /* !__cplusplus */
/** C **/

// C includes.
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#if 0
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#endif /* __cplusplus */

// MSVC intrinsics
#if defined(_MSC_VER) && _MSC_VER >= 1400
//# include <intrin.h>
#endif

// libwin32common
#ifdef _WIN32
//# include "libwin32common/RpWin32_sdk.h"
#endif /* _WIN32 */

// Common headers
#include "common.h"
#include "ctypex.h"

// TODO: Move to librpcpu.
#include "librpbase/byteswap.h"
#include "librpbase/bitstuff.h"

#endif /* __ROMPROPERTIES_LIBRPFILE_STDAFX_H__ */
