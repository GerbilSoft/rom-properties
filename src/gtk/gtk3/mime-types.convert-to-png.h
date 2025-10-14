/******************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                            *
 * mime-types.convert-to-png.h: MIME types for "Convert to PNG".              *
 *                                                                            *
 * Copyright (c) 2022-2023 by David Korth.                                    *
 * SPDX-License-Identifier: GPL-2.0-or-later                                  *
 ******************************************************************************/

#pragma once

// Supported MIME types
// NOTE: Must be sorted alphabetically for use with std::binary_search().
// TODO: Consolidate with the KF5 service menu?
static const char *const mime_types_convert_to_png[] = {
	"application/ico",
	"application/tga",
	"application/x-targa",
	"application/x-tga",
	"image/astc",
	"image/ico",
	"image/icon",
	"image/ktx",
	"image/ktx2",
	"image/qoi",
	"image/targa",
	"image/tga",
	"image/vnd.microsoft.cursor",
	"image/vnd.microsoft.icon",
	"image/vnd.ms-dds",
	"image/vnd.valve.source.texture",
	"image/x-cursor",
	"image/x-dds",
	"image/x-didj-texture",
	"image/x-godot-stex",
	"image/x-icb",
	"image/x-ico",
	"image/x-icon",
	"image/x-qoi",
	"image/x-sega-gvr",
	"image/x-sega-pvr",
	"image/x-sega-pvrx",
	"image/x-sega-svr",
	"image/x-targa",
	"image/x-tga",
	"image/x-vtf",
	"image/x-vtf3",
	"image/x-xbox-xpr0",
	"text/ico",
};
