/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * is-supported.hpp: Check if a URI is supported.                          *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifdef __cplusplus

namespace LibRpBase {
	class RomData;
};

/**
 * Attempt to open a RomData object from the specified GVfs URI.
 * If it is, the RomData object will be opened.
 * @param uri URI from e.g. nautilus_file_info_get_uri() [UTF-8]
 * @return RomData object if supported; nullptr if not.
 */
LibRpBase::RomData *rp_gtk_open_uri(const char *uri);

#endif /* __cplusplus */
