/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * dos_attrs.h: MS-DOS and Windows file attributes.                        *
 *                                                                         *
 * Copyright (c) 2022 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_DOS_ATTRS_H__
#define __ROMPROPERTIES_DOS_ATTRS_H__

// NOTE: These attributes use the same names as the Windows SDK.
// If the names are already defined, then we won't redefine them here.

// MS-DOS attributes
#ifndef FILE_ATTRIBUTE_READONLY
#define FILE_ATTRIBUTE_READONLY		0x01
#endif

#ifndef FILE_ATTRIBUTE_HIDDEN
#define FILE_ATTRIBUTE_HIDDEN		0x02
#endif

#ifndef FILE_ATTRIBUTE_SYSTEM
#define FILE_ATTRIBUTE_SYSTEM		0x04
#endif

#ifndef FILE_ATTRIBUTE_VOLUME
// Volume Label (not normally seen)
#define FILE_ATTRIBUTE_VOLUME		0x08
#endif

#ifndef FILE_ATTRIBUTE_DIRECTORY
#define FILE_ATTRIBUTE_DIRECTORY	0x10
#endif

#ifndef FILE_ATTRIBUTE_ARCHIVE
#define FILE_ATTRIBUTE_ARCHIVE		0x20
#endif

#ifndef FILE_ATTRIBUTE_DEVICE
#define FILE_ATTRIBUTE_DEVICE		0x40
#endif

#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL		0x80
#endif

// Windows NT attributes
#ifndef FILE_ATTRIBUTE_TEMPORARY
#define FILE_ATTRIBUTE_TEMPORARY	0x100
#endif

#ifndef FILE_ATTRIBUTE_SPARSE_FILE
#define FILE_ATTRIBUTE_SPARSE_FILE	0x200
#endif

#ifndef FILE_ATTRIBUTE_REPARSE_POINT
#define FILE_ATTRIBUTE_REPARSE_POINT	0x400
#endif

#ifndef FILE_ATTRIBUTE_COMPRESSED
#define FILE_ATTRIBUTE_COMPRESSED	0x800
#endif

#ifndef FILE_ATTRIBUTE_OFFLINE
#define FILE_ATTRIBUTE_OFFLINE		0x1000
#endif

#ifndef FILE_ATTRIBUTE_NOT_CONTENT_INDEXED
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED	0x2000
#endif

#ifndef FILE_ATTRIBUTE_ENCRYPTED
#define FILE_ATTRIBUTE_ENCRYPTED	0x4000
#endif

#ifndef FILE_ATTRIBUTE_INTEGRITY_STREAM
#define FILE_ATTRIBUTE_INTEGRITY_STREAM	0x8000
#endif

#ifndef FILE_ATTRIBUTE_VIRTUAL
#define FILE_ATTRIBUTE_VIRTUAL		0x10000
#endif

#ifndef FILE_ATTRIBUTE_NO_SCRUB_DATA
#define FILE_ATTRIBUTE_NO_SCRUB_DATA	0x20000
#endif

#ifndef FILE_ATTRIBUTE_RECALL_ON_OPEN
#define FILE_ATTRIBUTE_RECALL_ON_OPEN	0x40000
#endif

#ifndef FILE_ATTRIBUTE_PINNED
#define FILE_ATTRIBUTE_PINNED		0x80000
#endif

#ifndef FILE_ATTRIBUTE_UNPINNED
#define FILE_ATTRIBUTE_UNPINNED		0x100000
#endif

#ifndef FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS
#define FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS	0x400000
#endif

#ifndef FILE_ATTRIBUTE_VALID_FLAGS
#define FILE_ATTRIBUTE_VALID_FLAGS	0x7FB7
#endif

#ifndef FILE_ATTRIBUTE_VALID_SET_FLAGS
#define FILE_ATTRIBUTE_VALID_SET_FLAGS	0x31A7
#endif

#endif /* __ROMPROPERTIES_EXT2_FLAGS_H__ */
