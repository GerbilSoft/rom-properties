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
// NOTE: This defines _POSIX_C_SOURCE, which is
// required for *_r() functions on MinGW-w64,
// so it needs to be before other includes.
#include "time_r.h"

#ifdef __cplusplus
// C includes.
# include <sys/types.h>
// C includes. (C++ namespace)
# include <cassert>
# include <cstdio>
# include <cstring>
// C++ includes.
# include <algorithm>
# include <fstream>
# include <iomanip>
# include <iostream>
# include <locale>
# include <memory>
# include <vector>
#else /* !__cplusplus */
// C includes.
# include <assert.h>
# include <stdio.h>
# include <string.h>
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_RPCLI_STDAFX_H__ */
