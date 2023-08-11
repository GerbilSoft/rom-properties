/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * Ext2AttrData.h: Ext2 file system attribute data                         *
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
	chkAppendOnly,
	chkNoATime,
	chkCompressed,
	chkNoCOW,

	chkNoDump,
	chkDirSync,
	chkExtents,
	chkEncrypted,

	chkCasefold,
	chkImmutable,
	chkIndexed,
	chkJournalled,

	chkNoCompress,
	chkInlineData,
	chkProject,
	chkSecureDelete,

	chkFileSync,
	chkNoTailMerge,
	chkTopDir,
	chkUndelete,

	chkDAX,
	chkVerity,

	// NOTE: The number of attributes is not expected to change
	// anytime soon, so this constant can be used directly instead
	// of using a function to retrieve the highest attribute value.
	EXT2_ATTR_CHECKBOX_MAX
} Ext2AttrCheckboxID;

typedef struct _Ext2AttrCheckboxInfo_t {
	const char *name;	// object name
	const char *label;	// label (translatable)
	const char *tooltip;	// tooltip (translatable)
} Ext2AttrCheckboxInfo_t;

/**
 * Get Ext2 attribute checkbox info.
 *
 * NOTE: The UI frontend has to translate the label and tooltip
 * using the "Ext2AttrView" context.
 *
 * @param id Ext2AttrCheckboxID
 * @return Ext2AttrCheckboxInfo_t struct, or nullptr if the ID is invalid.
 */
RP_LIBROMDATA_PUBLIC
const Ext2AttrCheckboxInfo_t *ext2AttrCheckboxInfo(Ext2AttrCheckboxID id);

#ifdef __cplusplus
}
#endif
