/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * GdkImageConv_ifunc.cpp: GdkImageConv IFUNC resolution functions.        *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpcpuid.h"

#ifdef HAVE_IFUNC

#include "GdkImageConv.hpp"
using LibRpTexture::rp_image;

// NOTE: llvm/clang 14.0.0 fails to detect the resolver functions
// if they're marked static, even though the docs say this is okay.
// In C code, it merely warns that the resolvers aren't used, but
// in C++ code, the IFUNC_ATTR() attribute fails.

// IFUNC attribute doesn't support C++ name mangling.
extern "C" {

/**
 * IFUNC resolver function for rp_image_to_GdkPixbuf().
 * @return Function pointer.
 */
NO_SANITIZE_ADDRESS
__typeof__(&GdkImageConv::rp_image_to_GdkPixbuf_cpp) rp_image_to_GdkPixbuf_resolve(void)
{
	// NOTE: Since libromdata is a shared library now, IFUNC resolvers
	// cannot call PLT functions. Otherwise, it will crash.
	// We'll use gcc's built-in CPU ID functions instead.
	// Requires gcc-4.8 or later, or clang-6.0 or later.

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

#endif /* HAVE_IFUNC */
