/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * stdafx.h: Common definitions and includes.                              *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_STDAFX_H__
#define __ROMPROPERTIES_LIBRPBASE_STDAFX_H__

// time_r.h needs to be here due to *_r() issues on MinGW-w64.
#include "time_r.h"

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

namespace LibRpBase {

/**
 * NOTE: MSVC and gcc don't detect these operator<<() functions unless
 * they're defined within the namespace they're used in...
 */

/**
 * ostream operator<<() for char8_t
 */
static inline ::std::ostream& operator<<(::std::ostream& os, const char8_t *str)
{
	return os << reinterpret_cast<const char*>(str);
};

/**
 * ostream operator<<() for u8string
 */
static inline ::std::ostream& operator<<(::std::ostream& os, const std::u8string& str)
{
	return os << reinterpret_cast<const char*>(str.c_str());
};

}

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

// librpcpu
#include "librpcpu/byteswap_rp.h"
#include "librpcpu/bitstuff.h"

// librpbase common headers
#include "common.h"
#include "aligned_malloc.h"
#include "ctypex.h"
#include "dll-macros.h"
#include "librpcpu/cpu_dispatch.h"

#ifdef __cplusplus
// librpbase C++ headers
#include "TextFuncs.hpp"
#include "TextFuncs_printf.hpp"

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

// librpfile C++ headers
#include "librpfile/RpFile.hpp"
#endif /* !__cplusplus */

#endif /* __ROMPROPERTIES_LIBRPBASE_STDAFX_H__ */
