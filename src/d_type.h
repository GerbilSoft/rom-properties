/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * d_type.h: d_type enumeration.                                           *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_D_TYPE_H__
#define __ROMPROPERTIES_D_TYPE_H__

#ifndef _WIN32
#  include <dirent.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Get DT_* values from struct stat::st_mode
// and vice-versa.
#ifndef S_IFMT
#  define S_IFMT 00170000
#endif
#define DT2IF(dt) (((dt) << 12) & S_IFMT)
#define IF2DT(sif) (((sif) & S_IFMT) >> 12)

// Directory type values.
// From glibc-2.23's dirent.h.
enum {
#ifndef DT_UNKNOWN
    DT_UNKNOWN = 0,
# define DT_UNKNOWN	DT_UNKNOWN
#endif
#ifndef DT_FIFO
    DT_FIFO = 1,
# define DT_FIFO	DT_FIFO
#endif
#ifndef DT_CHR
    DT_CHR = 2,
# define DT_CHR		DT_CHR
#endif
#ifndef DT_DIR
    DT_DIR = 4,
# define DT_DIR		DT_DIR
#endif
#ifndef DT_BLK
    DT_BLK = 6,
# define DT_BLK		DT_BLK
#endif
#ifndef DT_REG
    DT_REG = 8,
# define DT_REG		DT_REG
#endif
#ifndef DT_LNK
    DT_LNK = 10,
# define DT_LNK		DT_LNK
#endif
#ifndef DT_SOCK
    DT_SOCK = 12,
# define DT_SOCK	DT_SOCK
#endif
#ifndef DT_WHT
    DT_WHT = 14,
# define DT_WHT		DT_WHT
#endif

    // Dummy entry to suppress warnings.
    DT_DUMMY = 12345,
};

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_D_TYPE_H__ */
