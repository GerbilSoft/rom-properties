/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * GdkImageConv_ifunc.cpp: GdkImageConv IFUNC resolution functions.        *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpbase.h"
#include "cpu_dispatch.h"

#ifdef RP_HAS_IFUNC

#include "GdkImageConv.hpp"
using LibRpTexture::rp_image;

// IFUNC attribute doesn't support C++ name mangling.
extern "C" {

/**
 * IFUNC resolver function for rp_image_to_GdkPixbuf().
 * @return Function pointer.
 */
static __typeof__(&GdkImageConv::rp_image_to_GdkPixbuf_cpp) rp_image_to_GdkPixbuf_resolve(void)
{
#ifdef GDKIMAGECONV_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return &GdkImageConv::rp_image_to_GdkPixbuf_ssse3;
	} else
#endif /* GDKIMAGECONV_HAS_SSSE3 */
	{
		return &GdkImageConv::rp_image_to_GdkPixbuf_cpp;
	}
}

}

GdkPixbuf *GdkImageConv::rp_image_to_GdkPixbuf(const rp_image *img)
	IFUNC_ATTR(rp_image_to_GdkPixbuf_resolve);

#endif /* RP_HAS_IFUNC */
