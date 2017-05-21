/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * d_type.h: d_type enumeration.                                           *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_DISC_D_TYPE_H__
#define __ROMPROPERTIES_LIBRPBASE_DISC_D_TYPE_H__

#ifndef _WIN32
#include <dirent.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

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
    DT_WHT = 14
# define DT_WHT		DT_WHT
#endif
};

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBRPBASE_DISC_D_TYPE_H__ */
