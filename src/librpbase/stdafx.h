/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * stdafx.h: Common definitions and includes.                              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// time_r.h needs to be here due to *_r() issues on MinGW-w64.
#include "time_r.h"

#ifdef __cplusplus
/** C++ **/

// C includes (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

// C++ includes
#include <algorithm>
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

// libfmt
#include <fmt/core.h>
#include <fmt/format.h>
#define FSTR FMT_STRING

#else /* !__cplusplus */
/** C **/

// C includes
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

// librpbyteswap
#include "librpbyteswap/byteswap_rp.h"
#include "librpbyteswap/bitstuff.h"

// librpbase common headers
#include "common.h"
#include "aligned_malloc.h"
#include "ctypex.h"
#include "dll-macros.h"

#ifdef __cplusplus
// librpbase C++ headers
#include "librptext/conversion.hpp"
#include "librptext/printf.hpp"

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

// librpfile C++ headers
#include "librpfile/RpFile.hpp"
#endif /* !__cplusplus */
