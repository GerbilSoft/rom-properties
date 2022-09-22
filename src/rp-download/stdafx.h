/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * stdafx.h: Common definitions and includes for COM.                      *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RP_DOWNLOAD_STDAFX_H__
#define __ROMPROPERTIES_RP_DOWNLOAD_STDAFX_H__

#ifdef _WIN32
// Windows SDK defines and includes.
#include "libwin32common/RpWin32_sdk.h"

// Additional Windows headers.
#include <shlobj.h>
#include <tchar.h>
#endif /* _WIN32 */

#ifdef __cplusplus
/** C++ **/

// C includes.
#include <stdint.h>
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <string>

#else /* !__cplusplus */
/** C **/

// C includes.
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#endif /* __cplusplus */

#include "common.h"
#include "ctypex.h"
#include "tcharx.h"

#endif /* __ROMPROPERTIES_RP_DOWNLOAD_STDAFX_H__ */
