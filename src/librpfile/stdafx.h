/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * stdafx.h: Common definitions and includes.                              *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPFILE_STDAFX_H__
#define __ROMPROPERTIES_LIBRPFILE_STDAFX_H__

#ifdef __cplusplus
/** C++ **/

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <stdint.h>

// C++ includes.
#include <algorithm>
#include <array>
#include <limits>
#include <memory>

#else /* !__cplusplus */
/** C **/

// C includes.
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

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

#endif /* __ROMPROPERTIES_LIBRPFILE_STDAFX_H__ */
