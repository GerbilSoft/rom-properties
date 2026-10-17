/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * is-supported.hpp: Check if a URI is supported.                          *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifdef __cplusplus

// librpbase
#include "librpbase/RomData.hpp"

/**
 * Attempt to open a RomData object from the specified GVfs URI.
 * @param uri URI from e.g. nautilus_file_info_get_uri() [UTF-8]
 * @param attrs Check for specific attributes. (If RDA_HAS_THUMBNAIL or RDA_HAS_METADATA are set, handles "bad FS" checking.)
 * @return RomData object if supported; nullptr if not.
 */
LibRpBase::RomDataPtr rp_gtk_open_uri(const char *uri, unsigned int attrs = 0);

#endif /* __cplusplus */
