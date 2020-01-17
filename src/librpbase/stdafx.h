/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * stdafx.h: Common definitions and includes.                              *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_STDAFX_H__
#define __ROMPROPERTIES_LIBRPBASE_STDAFX_H__

#ifdef __cplusplus
/** C++ **/

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <stdint.h>
#include <stdlib.h>

// C++ includes.
#include <algorithm>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#else /* !__cplusplus */
/** C **/

// C includes.
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#endif /* __cplusplus */

// MSVC intrinsics
#if defined(_MSC_VER) && _MSC_VER >= 1400
# include <intrin.h>
#endif

// libwin32common
#ifdef _WIN32
# include "libwin32common/RpWin32_sdk.h"
#endif /* _WIN32 */

// librpbase common headers
#include "common.h"
#include "byteswap.h"
#include "bitstuff.h"
#include "aligned_malloc.h"
#include "cpu_dispatch.h"
#include "ctypex.h"

#ifdef __cplusplus
// librpbase C++ headers
#include "TextFuncs.hpp"
#include "file/RpFile.hpp"

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"
#endif /* !__cplusplus */

#endif /* __ROMPROPERTIES_LIBRPBASE_STDAFX_H__ */
