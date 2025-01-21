/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
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
#include <memory>
#include <numeric>
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

// libi18n
#include "libi18n/i18n.h"

// librpbase common headers
#include "common.h"
#include "aligned_malloc.h"
#include "ctypex.h"
#include "dll-macros.h"

// librpbyteswap
#include "librpbyteswap/byteswap_rp.h"
#include "librpbyteswap/bitstuff.h"

#ifdef __cplusplus
// librpbase C++ headers
#include "librpbase/RomData.hpp"
#include "librpbase/img/IconAnimData.hpp"

// Uninitialized vector class
#include "uvector.h"

// librpbase DiscReader
#include "librpbase/disc/IDiscReader.hpp"
#include "librpbase/disc/DiscReader.hpp"
#include "librpbase/disc/IPartition.hpp"
#include "librpbase/disc/PartitionFile.hpp"

// RomData private headers
#include "librpbase/RomData_p.hpp"

// librpfile C++ headers
#include "librpfile/FileSystem.hpp"
#include "librpfile/IRpFile.hpp"
#include "librpfile/RpFile.hpp"

// librptexture C++ headers
#include "librptexture/img/rp_image.hpp"
#include "librptexture/ImageSizeCalc.hpp"
#include "librptexture/decoder/PixelConversion.hpp"

// librptext C++ headers
#include "librptext/conversion.hpp"
#include "librptext/printf.hpp"
#endif /* !__cplusplus */
