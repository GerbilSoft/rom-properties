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
	{"chkAppendOnly", 'a', NOP_C_("Ext2AttrView", "append only"),
	 NOP_C_("Ext2AttrView", "File can only be opened in append mode for writing.")},

	{"chkNoATime", 'A', NOP_C_("Ext2AttrView", "no atime"),
	 NOP_C_("Ext2AttrView", "Access time record is not modified.")},

	{"chkCompressed", 'c', NOP_C_("Ext2AttrView", "compressed"),
	 NOP_C_("Ext2AttrView", "File is compressed.")},

	{"chkNoCOW", 'C', NOP_C_("Ext2AttrView", "no CoW"),
	 NOP_C_("Ext2AttrView", "Not subject to copy-on-write updates.")},

	{"chkNoDump", 'd', NOP_C_("Ext2AttrView", "no dump"),
	// tr: "dump" is the name of the executable, so it should not be localized.
	 NOP_C_("Ext2AttrView", "This file is not a candidate for dumping with the dump(8) program.")},

	{"chkDirSync", 'D', NOP_C_("Ext2AttrView", "dir sync"),
	 NOP_C_("Ext2AttrView", "Changes to this directory are written synchronously to the disk.")},

	{"chkExtents", 'e', NOP_C_("Ext2AttrView", "extents"),
	 NOP_C_("Ext2AttrView", "File is mapped on disk using extents.")},

	{"chkEncrypted", 'E', NOP_C_("Ext2AttrView", "encrypted"),
	 NOP_C_("Ext2AttrView", "File is encrypted.")},

	{"chkCasefold", 'F', NOP_C_("Ext2AttrView", "casefold"),
	 NOP_C_("Ext2AttrView", "Files stored in this directory use case-insensitive filenames.")},

	{"chkImmutable", 'i', NOP_C_("Ext2AttrView", "immutable"),
	 NOP_C_("Ext2AttrView", "File cannot be modified, deleted, or renamed.")},

	{"chkIndexed", 'I', NOP_C_("Ext2AttrView", "indexed"),
	 NOP_C_("Ext2AttrView", "Directory is indexed using hashed trees.")},

	{"chkJournalled", 'j', NOP_C_("Ext2AttrView", "journalled"),
	 NOP_C_("Ext2AttrView", "File data is written to the journal before writing to the file itself.")},

	{"chkNoCompress", 'm', NOP_C_("Ext2AttrView", "no compress"),
	 NOP_C_("Ext2AttrView", "File is excluded from compression.")},

	{"chkInlineData", 'N', NOP_C_("Ext2AttrView", "inline data"),
	 NOP_C_("Ext2AttrView", "File data is stored inline in the inode.")},

	{"chkProject", 'P', NOP_C_("Ext2AttrView", "project"),
	 NOP_C_("Ext2AttrView", "Directory will enforce a hierarchical structure for project IDs.")},

	{"chkSecureDelete", 's', NOP_C_("Ext2AttrView", "secure del"),
	 NOP_C_("Ext2AttrView", "File's blocks will be zeroed when deleted.")},

	{"chkFileSync", 'S', NOP_C_("Ext2AttrView", "sync"),
	 NOP_C_("Ext2AttrView", "Changes to this file are written synchronously to the disk.")},

	{"chkNoTailMerge", 't', NOP_C_("Ext2AttrView", "no tail merge"),
	 NOP_C_("Ext2AttrView", "If the file system supports tail merging, this file will not have a partial block fragment at the end of the file merged with other files.")},

	{"chkTopDir", 'T', NOP_C_("Ext2AttrView", "top dir"),
	 NOP_C_("Ext2AttrView", "Directory will be treated like a top-level directory by the ext3/ext4 Orlov block allocator.")},

	{"chkUndelete", 'u', NOP_C_("Ext2AttrView", "undelete"),
	 NOP_C_("Ext2AttrView", "File's contents will be saved when deleted, potentially allowing for undeletion. This is known to be broken.")},

	{"chkDAX", 'x', NOP_C_("Ext2AttrView", "DAX"),
	 NOP_C_("Ext2AttrView", "Direct access")},

	{"chkVerity", 'V', NOP_C_("Ext2AttrView", "fs-verity"),
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
