/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * stdafx.h: Common definitions and includes.                              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifdef __cplusplus
/** C++ **/

// C includes (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// C++ includes
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

// libfmt
#include "rp-libfmt.h"

#else /* !__cplusplus */
/** C **/

// C includes
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

// librpbyteswap
#include "librpbyteswap/byteswap_rp.h"
#include "librpbyteswap/bitstuff.h"

#ifdef __cplusplus
// C++ headers
#include "uvector.h"

// librpbase C++ headers
#include "librpbase/RomFields.hpp"

// librpfile C++ headers
#include "librpfile/IRpFile.hpp"
#include "librpfile/FileSystem.hpp"

// librptext C++ headers
#include "librptext/conversion.hpp"
#endif /* !__cplusplus */
