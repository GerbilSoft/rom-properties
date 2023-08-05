/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * stdafx.h: Common definitions and includes.                              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifdef __cplusplus
/** C++ **/

// C includes (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <stdint.h>

#ifdef _WIN32
#  include <cwctype>
#endif /* _WIN32 */

// C++ includes
#include <algorithm>
#include <array>
#include <limits>
#include <memory>

#else /* !__cplusplus */
/** C **/

// C includes
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#ifdef _WIN32
#  include <wctype.h>
#endif /* _WIN32 */

#endif /* __cplusplus */

// libwin32common
#ifdef _WIN32
# include "libwin32common/RpWin32_sdk.h"
#endif /* _WIN32 */

// Common headers
#include "common.h"
#include "ctypex.h"

// librpcpu
#include "librpcpu/byteswap_rp.h"
#include "librpcpu/bitstuff.h"
