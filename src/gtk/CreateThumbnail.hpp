/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CreateThumbnail.hpp: Thumbnail creator for wrapper programs.            *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CREATETHUMBNAIL_HPP__
#define __ROMPROPERTIES_GTK_CREATETHUMBNAIL_HPP__

#include <gmodule.h>

// NOTE: G_MODULE_EXPORT is a no-op on non-Windows platforms.
#if !defined(_WIN32) && defined(__GNUC__) && __GNUC__ >= 4
#undef G_MODULE_EXPORT
#define G_MODULE_EXPORT __attribute__ ((visibility ("default")))
#endif

G_BEGIN_DECLS;

/**
 * rp_create_thumbnail() errors.
 */
typedef enum {
	RPCT_SUCCESS			= 0,
	RPCT_DLL_ERROR			= 1,	// Cannot load the shared library.
	RPCT_SOURCE_FILE_ERROR		= 2,	// Cannot open the source file.
	RPCT_SOURCE_FILE_NOT_SUPPORTED	= 3,	// Source file isn't supported.
	RPCT_SOURCE_FILE_NO_IMAGE	= 4,	// Source file has no image.
	RPCT_OUTPUT_FILE_FAILED		= 5,	// Failed to save the output file.
} RpCreateThumbnailError;

// Thumbnail nearest-neighbor upscaling policy.
// TODO: Make this configurable.
// TODO: Combine with win32/RP_ThumbnailProvider and kde/RomThumbCreator.
// (originally called ResizePolicy)
typedef enum {
	RESIZE_UP_NONE,	// No resizing.

	// Only resize images that are less than or equal to half the
	// requested thumbnail size. This is a compromise to allow
	// small icons like Nintendo DS icons to be enlarged while
	// larger but not-quite 256px images like GameTDB disc scans'
	// (160px) will remain as-is.
	RESIZE_UP_HALF,

	// Resize all images that are smaller than the requested
	// thumbnail size.
	RESIZE_UP_ALL,
} ResizeNearestUpPolicy;

/**
 * Thumbnail creator function for wrapper programs.
 * @param source_file Source file. (UTF-8)
 * @param output_file Output file. (UTF-8)
 * @param maximum_size Maximum size.
 * @return 0 on success; non-zero on error.
 */
G_MODULE_EXPORT int rp_create_thumbnail(const char *source_file, const char *output_file, int maximum_size);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_GTK_CREATETHUMBNAIL_HPP__ */
