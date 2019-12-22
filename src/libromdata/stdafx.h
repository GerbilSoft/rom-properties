/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * stdafx.h: Common definitions and includes.                              *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_STDAFX_H__
#define __ROMPROPERTIES_LIBROMDATA_STDAFX_H__

#ifdef __cplusplus
/** C++ **/

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <stdlib.h>

// C++ includes.
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

#else /* !__cplusplus */
/** C **/

// C includes.
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#endif /* __cplusplus */

#if 0
// MSVC intrinsics
#if defined(_MSC_VER) && _MSC_VER >= 1400
# include <intrin.h>
#endif

// libwin32common
#ifdef _WIN32
# include "libwin32common/RpWin32_sdk.h"
#endif /* _WIN32 */
#endif

// libi18n
#include "libi18n/i18n.h"

// librpbase common headers
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/bitstuff.h"
#include "librpbase/aligned_malloc.h"
#include "librpbase/ctypex.h"

// librpbase C++ headers
#ifdef __cplusplus
#include "librpbase/RomData.hpp"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "librpbase/file/FileSystem.hpp"
#include "librpbase/img/IconAnimData.hpp"

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "librpbase/uvector.h"

// librpbase DiscReader
#include "librpbase/disc/IDiscReader.hpp"
#include "librpbase/disc/DiscReader.hpp"
#include "librpbase/disc/IPartition.hpp"
#include "librpbase/disc/PartitionFile.hpp"

// RomData private headers
#include "librpbase/RomData_p.hpp"

// librptexture C++ headers
#include "librptexture/img/rp_image.hpp"
#include "librptexture/decoder/ImageDecoder.hpp"
#endif /* !__cplusplus */

#endif /* __ROMPROPERTIES_LIBROMDATA_STDAFX_H__ */
