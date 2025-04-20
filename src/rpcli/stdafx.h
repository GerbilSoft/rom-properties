/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * stdafx.h: Common includes.                                              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Time functions, with workaround for systems
// that don't have reentrant versions.
// NOTE: On Windows, this defines _POSIX_C_SOURCE, which is
// required for *_r() functions on MinGW-w64, so it needs to
// be included before anything else.
#include "time_r.h"

#ifdef __cplusplus
/** C++ **/

// C includes
#  include <sys/types.h>

// C includes (C++ namespace)
#  include <cassert>
#  include <cerrno>
#  include <cstdio>
#  include <cstdlib>
#  include <cstring>

// C++ includes
#  include <algorithm>
#  include <array>
#  include <fstream>
#  include <iomanip>
#  include <iostream>
#  include <locale>
#  include <memory>
#  include <string>
#  include <vector>

// libfmt
#ifndef RP_NO_INCLUDE_LIBFMT_IN_STDAFX_H
#  include "rp-libfmt.h"
#endif

#else /* !__cplusplus */
/** C **/

// C includes
#  include <assert.h>
#  include <errno.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>

#endif /* __cplusplus */
