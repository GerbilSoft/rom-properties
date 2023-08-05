/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * LinuxAttrData.c: Linux file system attribute data                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "LinuxAttrData.h"

#include "common.h"
#include "libi18n/i18n.h"

// C includes
#include <assert.h>

static const LinuxAttrCheckboxInfo_t linuxAttrCheckboxInfo_data[LINUX_ATTR_CHECKBOX_MAX] = {
	{"chkAppendOnly", NOP_C_("LinuxAttrView", "a: append only"),
	 NOP_C_("LinuxAttrView", "File can only be opened in append mode for writing.")},

	{"chkNoATime", NOP_C_("LinuxAttrView", "A: no atime"),
	 NOP_C_("LinuxAttrView", "Access time record is not modified.")},

	{"chkCompressed", NOP_C_("LinuxAttrView", "c: compressed"),
	 NOP_C_("LinuxAttrView", "File is compressed.")},

	{"chkNoCOW", NOP_C_("LinuxAttrView", "C: no CoW"),
	 NOP_C_("LinuxAttrView", "Not subject to copy-on-write updates.")},

	{"chkNoDump", NOP_C_("LinuxAttrView", "d: no dump"),
	// tr: "dump" is the name of the executable, so it should not be localized.
	 NOP_C_("LinuxAttrView", "This file is not a candidate for dumping with the dump(8) program.")},

	{"chkDirSync", NOP_C_("LinuxAttrView", "D: dir sync"),
	 NOP_C_("LinuxAttrView", "Changes to this directory are written synchronously to the disk.")},

	{"chkExtents", NOP_C_("LinuxAttrView", "e: extents"),
	 NOP_C_("LinuxAttrView", "File is mapped on disk using extents.")},

	{"chkEncrypted", NOP_C_("LinuxAttrView", "E: encrypted"),
	 NOP_C_("LinuxAttrView", "File is encrypted.")},

	{"chkCasefold", NOP_C_("LinuxAttrView", "F: casefold"),
	 NOP_C_("LinuxAttrView", "Files stored in this directory use case-insensitive filenames.")},

	{"chkImmutable", NOP_C_("LinuxAttrView", "i: immutable"),
	 NOP_C_("LinuxAttrView", "File cannot be modified, deleted, or renamed.")},

	{"chkIndexed", NOP_C_("LinuxAttrView", "I: indexed"),
	 NOP_C_("LinuxAttrView", "Directory is indexed using hashed trees.")},

	{"chkJournalled", NOP_C_("LinuxAttrView", "j: journalled"),
	 NOP_C_("LinuxAttrView", "File data is written to the journal before writing to the file itself.")},

	{"chkNoCompress", NOP_C_("LinuxAttrView", "m: no compress"),
	 NOP_C_("LinuxAttrView", "File is excluded from compression.")},

	{"chkInlineData", NOP_C_("LinuxAttrView", "N: inline data"),
	 NOP_C_("LinuxAttrView", "File data is stored inline in the inode.")},

	{"chkProject", NOP_C_("LinuxAttrView", "P: project"),
	 NOP_C_("LinuxAttrView", "Directory will enforce a hierarchical structure for project IDs.")},

	{"chkSecureDelete", NOP_C_("LinuxAttrView", "s: secure del"),
	 NOP_C_("LinuxAttrView", "File's blocks will be zeroed when deleted.")},

	{"chkFileSync", NOP_C_("LinuxAttrView", "S: sync"),
	 NOP_C_("LinuxAttrView", "Changes to this file are written synchronously to the disk.")},

	{"chkNoTailMerge", NOP_C_("LinuxAttrView", "t: no tail merge"),
	 NOP_C_("LinuxAttrView", "If the file system supports tail merging, this file will not have a partial block fragment at the end of the file merged with other files.")},

	{"chkTopDir", NOP_C_("LinuxAttrView", "T: top dir"),
	 NOP_C_("LinuxAttrView", "Directory will be treated like a top-level directory by the ext3/ext4 Orlov block allocator.")},

	{"chkUndelete", NOP_C_("LinuxAttrView", "u: undelete"),
	 NOP_C_("LinuxAttrView", "File's contents will be saved when deleted, potentially allowing for undeletion. This is known to be broken.")},

	{"chkDAX", NOP_C_("LinuxAttrView", "x: DAX"),
	 NOP_C_("LinuxAttrView", "Direct access")},

	{"chkVerity", NOP_C_("LinuxAttrView", "V: fs-verity"),
	 NOP_C_("LinuxAttrView", "File has fs-verity enabled.")},
};

/**
 * Get Linux attribute checkbox info.
 *
 * NOTE: The UI frontend has to translate the label and tooltip
 * using the "LinuxAttrView" context.
 *
 * @param id LinuxAttrCheckboxID
 * @return LinuxAttrCheckboxInfo_t struct, or nullptr if the ID is invalid.
 */
const LinuxAttrCheckboxInfo_t *linuxAttrCheckboxInfo(LinuxAttrCheckboxID id)
{
	static_assert(ARRAY_SIZE(linuxAttrCheckboxInfo_data) == LINUX_ATTR_CHECKBOX_MAX, "linuxAttrCheckboxInfo_data[] size is incorrect");

	assert(id >= chkAppendOnly);
	assert(id < LINUX_ATTR_CHECKBOX_MAX);
	if (id < chkAppendOnly || id >= LINUX_ATTR_CHECKBOX_MAX) {
		return NULL;
	}

	return &linuxAttrCheckboxInfo_data[id];
}
