/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * is-supported.hpp: Check if a URI is supported.                          *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_IS_SUPPORTED_H__
#define __ROMPROPERTIES_GTK_IS_SUPPORTED_H__

#include <glib.h>

#ifdef __cplusplus
namespace LibRomData {
	class RomData;
};

/**
 * Attempt to open a RomData object from the specified GVfs URI.
 * If it is, the RomData object will be opened.
 * @param uri URI rom e.g. nautilus_file_info_get_uri().
 * @return RomData object if supported; nullptr if not.
 */
LibRpBase::RomData *rp_gtk_open_uri(const gchar *uri);

#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_GTK_IS_SUPPORTED_H__ */
