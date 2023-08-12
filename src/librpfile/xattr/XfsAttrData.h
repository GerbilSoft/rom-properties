/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * XfsAttrData.h: XFS file system attribute data                           *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	XFS_chkRealtime,
	XFS_chkPrealloc,
	XFS_chkImmutable,
	XFS_chkAppend,

	XFS_chkSync,
	XFS_chkNoATime,
	XFS_chkNoDump,
	XFS_chkRtInherit,

	XFS_chkProjInherit,
	XFS_chkNoSymlinks,
	XFS_chkExtSize,
	XFS_chkExtSzInherit,

	XFS_chkNoDefrag,
	XFS_chkFilestream,
	XFS_chkHasAttr,

	// NOTE: The number of attributes is not expected to change
	// anytime soon, so this constant can be used directly instead
	// of using a function to retrieve the highest attribute value.
	XFS_ATTR_CHECKBOX_MAX
} XfsAttrCheckboxID;

typedef struct _XfsAttrCheckboxInfo_t {
	const char *name;	// object name
	const char *label;	// label (translatable)
	const char *tooltip;	// tooltip (translatable)
} XfsAttrCheckboxInfo_t;

/**
 * Get XFS attribute checkbox info.
 *
 * NOTE: The UI frontend has to translate the label and tooltip
 * using the "XfsAttrView" context.
 *
 * @param id XfsAttrCheckboxID
 * @return XfsAttrCheckboxInfo_t struct, or nullptr if the ID is invalid.
 */
RP_LIBROMDATA_PUBLIC
const XfsAttrCheckboxInfo_t *xfsAttrCheckboxInfo(XfsAttrCheckboxID id);

#ifdef __cplusplus
}
#endif
