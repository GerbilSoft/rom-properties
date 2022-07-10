/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * stdafx.h: Common includes.                                              *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RPCLI_STDAFX_H__
#define __ROMPROPERTIES_RPCLI_STDAFX_H__

// Time functions, with workaround for systems
// that don't have reentrant versions.
// NOTE: On Windows, this defines _POSIX_C_SOURCE, which is
// required for *_r() functions on MinGW-w64, so it needs to
// be included before anything else.
#include "time_r.h"

#ifdef __cplusplus

// C includes
#include <sys/types.h>
// C includes (C++ namespace)
#include <cassert>
#include <cstdio>
#include <cstring>
// C++ includes
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <memory>
#include <vector>

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

#else /* !__cplusplus */

// C includes
#include <assert.h>
#include <stdio.h>
#include <string.h>

#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_RPCLI_STDAFX_H__ */
