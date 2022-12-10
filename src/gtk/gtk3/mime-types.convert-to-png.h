/******************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                            *
 * mime-types.convert-to-png.h: MIME types for "Convert to PNG".              *
 *                                                                            *
 * Copyright (c) 2022 by David Korth.                                         *
 * SPDX-License-Identifier: GPL-2.0-or-later                                  *
 ******************************************************************************/

#ifndef __ROMPROPERTIES_GTK3_MIMETYPES_CONVERTTOPNG_HPP__
#define __ROMPROPERTIES_GTK3_MIMETYPES_CONVERTTOPNG_HPP__

// Supported MIME types
// NOTE: Must be sorted alphabetically for use with std::binary_search().
// TODO: Consolidate with the KF5 service menu?
static const char *const mime_types_convert_to_png[] = {
	"image/astc",
	"image/ktx",
	"image/ktx2",
	"image/vnd.ms-dds",
	"image/vnd.valve.source.texture",
	"image/x-dds",
	"image/x-didj-texture",
	"image/x-godot-stex",
	"image/x-sega-gvr",
	"image/x-sega-pvr",
	"image/x-sega-pvrx",
	"image/x-sega-svr",
	"image/x-vtf",
	"image/x-vtf3",
	"image/x-xbox-xpr0",
};

#endif /* __ROMPROPERTIES_GTK3_MIMETYPES_CONVERTTOPNG_HPP__ */
