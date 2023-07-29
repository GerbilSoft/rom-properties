/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * d_type.h: d_type enumeration.                                           *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.libc.h"

#if defined(HAVE_DIRENT_H)
#  include <dirent.h>
#elif defined(HAVE_SYS_STAT_H)
// Windows Universal CRT has S_IFMT defined in <sys/stat.h>.
#  include <sys/stat.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Convert between stat structure types and directory types.  */
#ifndef S_IFMT
#  define S_IFMT 0170000
#endif
#ifndef IFTODT
#  define IFTODT(mode)    (((mode) & S_IFMT) >> 12)
#endif
#ifndef DTTOIF
#  define DTTOIF(dirtype) ((dirtype) << 12)
#endif

// Directory type values.
// From glibc-2.23's dirent.h.
enum {
#ifndef DT_UNKNOWN
    DT_UNKNOWN = 0,
#  define DT_UNKNOWN DT_UNKNOWN
#endif
#ifndef DT_FIFO
    DT_FIFO = 1,
#  define DT_FIFO DT_FIFO
#endif
#ifndef DT_CHR
    DT_CHR = 2,
#  define DT_CHR DT_CHR
#endif
#ifndef DT_DIR
    DT_DIR = 4,
#  define DT_DIR DT_DIR
#endif
#ifndef DT_BLK
    DT_BLK = 6,
#  define DT_BLK DT_BLK
#endif
#ifndef DT_REG
    DT_REG = 8,
#  define DT_REG DT_REG
#endif
#ifndef DT_LNK
    DT_LNK = 10,
#  define DT_LNK DT_LNK
#endif
#ifndef DT_SOCK
    DT_SOCK = 12,
#  define DT_SOCK DT_SOCK
#endif
#ifndef DT_WHT
    DT_WHT = 14,
#  define DT_WHT DT_WHT
#endif

    // Dummy entry to suppress warnings.
    DT_DUMMY = 12345,
};

#ifdef __cplusplus
}
#endif
