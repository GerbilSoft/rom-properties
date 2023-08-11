/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * Ext2AttrData.c: Ext2 file system attribute data                         *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Ext2AttrData.h"

#include "common.h"
#include "libi18n/i18n.h"

// C includes
#include <assert.h>

static const Ext2AttrCheckboxInfo_t ext2AttrCheckboxInfo_data[EXT2_ATTR_CHECKBOX_MAX] = {
	{"chkAppendOnly", NOP_C_("Ext2AttrView", "a: append only"),
	 NOP_C_("Ext2AttrView", "File can only be opened in append mode for writing.")},

	{"chkNoATime", NOP_C_("Ext2AttrView", "A: no atime"),
	 NOP_C_("Ext2AttrView", "Access time record is not modified.")},

	{"chkCompressed", NOP_C_("Ext2AttrView", "c: compressed"),
	 NOP_C_("Ext2AttrView", "File is compressed.")},

	{"chkNoCOW", NOP_C_("Ext2AttrView", "C: no CoW"),
	 NOP_C_("Ext2AttrView", "Not subject to copy-on-write updates.")},

	{"chkNoDump", NOP_C_("Ext2AttrView", "d: no dump"),
	// tr: "dump" is the name of the executable, so it should not be localized.
	 NOP_C_("Ext2AttrView", "This file is not a candidate for dumping with the dump(8) program.")},

	{"chkDirSync", NOP_C_("Ext2AttrView", "D: dir sync"),
	 NOP_C_("Ext2AttrView", "Changes to this directory are written synchronously to the disk.")},

	{"chkExtents", NOP_C_("Ext2AttrView", "e: extents"),
	 NOP_C_("Ext2AttrView", "File is mapped on disk using extents.")},

	{"chkEncrypted", NOP_C_("Ext2AttrView", "E: encrypted"),
	 NOP_C_("Ext2AttrView", "File is encrypted.")},

	{"chkCasefold", NOP_C_("Ext2AttrView", "F: casefold"),
	 NOP_C_("Ext2AttrView", "Files stored in this directory use case-insensitive filenames.")},

	{"chkImmutable", NOP_C_("Ext2AttrView", "i: immutable"),
	 NOP_C_("Ext2AttrView", "File cannot be modified, deleted, or renamed.")},

	{"chkIndexed", NOP_C_("Ext2AttrView", "I: indexed"),
	 NOP_C_("Ext2AttrView", "Directory is indexed using hashed trees.")},

	{"chkJournalled", NOP_C_("Ext2AttrView", "j: journalled"),
	 NOP_C_("Ext2AttrView", "File data is written to the journal before writing to the file itself.")},

	{"chkNoCompress", NOP_C_("Ext2AttrView", "m: no compress"),
	 NOP_C_("Ext2AttrView", "File is excluded from compression.")},

	{"chkInlineData", NOP_C_("Ext2AttrView", "N: inline data"),
	 NOP_C_("Ext2AttrView", "File data is stored inline in the inode.")},

	{"chkProject", NOP_C_("Ext2AttrView", "P: project"),
	 NOP_C_("Ext2AttrView", "Directory will enforce a hierarchical structure for project IDs.")},

	{"chkSecureDelete", NOP_C_("Ext2AttrView", "s: secure del"),
	 NOP_C_("Ext2AttrView", "File's blocks will be zeroed when deleted.")},

	{"chkFileSync", NOP_C_("Ext2AttrView", "S: sync"),
	 NOP_C_("Ext2AttrView", "Changes to this file are written synchronously to the disk.")},

	{"chkNoTailMerge", NOP_C_("Ext2AttrView", "t: no tail merge"),
	 NOP_C_("Ext2AttrView", "If the file system supports tail merging, this file will not have a partial block fragment at the end of the file merged with other files.")},

	{"chkTopDir", NOP_C_("Ext2AttrView", "T: top dir"),
	 NOP_C_("Ext2AttrView", "Directory will be treated like a top-level directory by the ext3/ext4 Orlov block allocator.")},

	{"chkUndelete", NOP_C_("Ext2AttrView", "u: undelete"),
	 NOP_C_("Ext2AttrView", "File's contents will be saved when deleted, potentially allowing for undeletion. This is known to be broken.")},

	{"chkDAX", NOP_C_("Ext2AttrView", "x: DAX"),
	 NOP_C_("Ext2AttrView", "Direct access")},

	{"chkVerity", NOP_C_("Ext2AttrView", "V: fs-verity"),
	 NOP_C_("Ext2AttrView", "File has fs-verity enabled.")},
};

/**
 * Get Ext2 attribute checkbox info.
 *
 * NOTE: The UI frontend has to translate the label and tooltip
 * using the "Ext2AttrView" context.
 *
 * @param id Ext2AttrCheckboxID
 * @return Ext2AttrCheckboxInfo_t struct, or nullptr if the ID is invalid.
 */
const Ext2AttrCheckboxInfo_t *ext2AttrCheckboxInfo(Ext2AttrCheckboxID id)
{
	static_assert(ARRAY_SIZE(ext2AttrCheckboxInfo_data) == EXT2_ATTR_CHECKBOX_MAX, "ext2AttrCheckboxInfo_data[] size is incorrect");

	assert(id >= chkAppendOnly);
	assert(id < EXT2_ATTR_CHECKBOX_MAX);
	if (id < chkAppendOnly || id >= EXT2_ATTR_CHECKBOX_MAX) {
		return NULL;
	}

	return &ext2AttrCheckboxInfo_data[id];
}
