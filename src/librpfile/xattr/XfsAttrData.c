/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * XfsAttrData.c: XFS file system attribute data                           *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XfsAttrData.h"

#include "common.h"
#include "libi18n/i18n.h"

// C includes
#include <assert.h>

static const XfsAttrCheckboxInfo_t xfsAttrCheckboxInfo_data[XFS_ATTR_CHECKBOX_MAX] = {
	{"chkRealtime", NOP_C_("XfsAttrView", "realtime"),
	 NOP_C_("XfsAttrView", "Data in realtime volume")},

	{"chkPrealloc", NOP_C_("XfsAttrView", "prealloc"),
	 NOP_C_("XfsAttrView", "Preallocated file extents")},

	{"chkImmutable", NOP_C_("XfsAttrView", "immutable"),
	 NOP_C_("XfsAttrView", "File cannot be modified")},

	{"chkAppend", NOP_C_("XfsAttrView", "append"),
	 NOP_C_("XfsAttrView", "All writes append")},

	{"chkSync", NOP_C_("XfsAttrView", "sync"),
	 NOP_C_("XfsAttrView", "All writes synchronous")},

	{"chkNoATime", NOP_C_("XfsAttrView", "no atime"),
	 NOP_C_("XfsAttrView", "Do not update access time")},

	{"chkNoDump", NOP_C_("XfsAttrView", "no dump"),
	 NOP_C_("XfsAttrView", "Do not include in backups")},

	{"chkRtInherit", NOP_C_("XfsAttrView", "RT inherit"),
	 NOP_C_("XfsAttrView", "Create with RT bit set")},

	{"chkProjInherit", NOP_C_("XfsAttrView", "proj inherit"),
	 NOP_C_("XfsAttrView", "Create with parent's project ID")},

	{"chkNoSymlinks", NOP_C_("XfsAttrView", "no symlinks"),
	 NOP_C_("XfsAttrView", "Disallow symlink creation")},

	{"chkExtSize", NOP_C_("XfsAttrView", "extsize"),
	 NOP_C_("XfsAttrView", "Extent size allocator hint")},

	{"chkExtSzInherit", NOP_C_("XfsAttrView", "extszinherit"),
	 NOP_C_("XfsAttrView", "Inherit inode extent size")},

	{"chkNoDefrag", NOP_C_("XfsAttrView", "no defrag"),
	 NOP_C_("XfsAttrView", "Do not defragment")},

	{"chkFilestream", NOP_C_("XfsAttrView", "filestream"),
	 NOP_C_("XfsAttrView", "Use filestream allocator")},

	{"chkHasAttr", NOP_C_("XfsAttrView", "hasattr"),
	 NOP_C_("XfsAttrView", "Has attributes")},
};

/**
 * Get XFS attribute checkbox info.
 *
 * NOTE: The UI frontend has to translate the label and tooltip
 * using the "XfsAttrView" context.
 *
 * @param id XfsAttrCheckboxID
 * @return XfsAttrCheckboxInfo_t struct, or nullptr if the ID is invalid.
 */
const XfsAttrCheckboxInfo_t *xfsAttrCheckboxInfo(XfsAttrCheckboxID id)
{
	static_assert(ARRAY_SIZE(xfsAttrCheckboxInfo_data) == XFS_ATTR_CHECKBOX_MAX, "xfsAttrCheckboxInfo_data[] size is incorrect");

	assert(id >= XFS_chkRealtime);
	assert(id < XFS_ATTR_CHECKBOX_MAX);
	if (id < XFS_chkRealtime || id >= XFS_ATTR_CHECKBOX_MAX) {
		return NULL;
	}

	return &xfsAttrCheckboxInfo_data[id];
}
