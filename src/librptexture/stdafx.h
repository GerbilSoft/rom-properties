/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * stdafx.h: Common definitions and includes.                              *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_STDAFX_H__
#define __ROMPROPERTIES_LIBRPTEXTURE_STDAFX_H__

#ifdef __cplusplus
/** C++ **/

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdlib.h>

// C++ includes.
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#else /* !__cplusplus */
/** C **/

// C includes.
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#endif /* __cplusplus */

// librpbase common headers
#include "common.h"
#include "ctypex.h"
#include "aligned_malloc.h"

// librpcpu
#include "librpcpu/byteswap_rp.h"
#include "librpcpu/bitstuff.h"

#ifdef __cplusplus
// C++ headers
#include "uvector.h"

// librpbase C++ headers
#include "librpbase/RomFields.hpp"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/TextFuncs_printf.hpp"

// librpfile C++ headers
#include "librpfile/IRpFile.hpp"
#include "librpfile/FileSystem.hpp"
#endif /* !__cplusplus */

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_STDAFX_H__ */
