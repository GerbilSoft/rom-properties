/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * stdafx.h: Common includes.                                              *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
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
// C includes. (C++ namespace)
#include <cassert>
#include <cstring>
// C++ includes.
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <memory>
#include <vector>
#else /* !__cplusplus */
// C includes.
#include <assert.h>
#include <string.h>
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_RPCLI_STDAFX_H__ */
