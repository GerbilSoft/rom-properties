/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * APNG_dlopen.h: APNG dlopen()'d function pointers.                       *
 *                                                                         *
 * Copyright (c) 2014-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/config.librpbase.h"

#include <png.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Load APNG and increment the reference counter.
 *
 * NOTE: On Windows, if libpng is set for delay-load, the caller
 * *must* ensure that it's loaded before calling this function!
 * Otherwise, this function will fail.
 *
 * @return 0 on success; non-zero on error.
 */
extern int RP_C_API APNG_ref(void);

/**
 * Decrement the APNG reference counter.
 */
extern void RP_C_API APNG_unref(void);

/**
 * Force the APNG library to be unloaded.
 * This decrements the reference count to 0.
 */
extern void APNG_force_unload(void);

#ifndef USE_INTERNAL_PNG
/* PNG/APNG macros that might not be present if the system libpng *
 * is too old, or if it doesn't have support for APNG.            */

/** PNG_CALLBACK was added in libpng-1.5.0beta14. [2010/03/14] **/
#ifndef PNGCAPI
#  define PNGCAPI
#endif
#ifndef PNGCBAPI
#  define PNGCBAPI PNGCAPI
#endif
#ifndef PNGAPI
#  define PNGAPI PNGCAPI
#endif
#ifndef PNG_CALLBACK
#  define PNG_CALLBACK(type, name, args) type (PNGCBAPI name) PNGARG(args)
#endif

#ifndef PNG_APNG_SUPPORTED
/* dispose_op flags from inside fcTL */
#define PNG_DISPOSE_OP_NONE        0x00
#define PNG_DISPOSE_OP_BACKGROUND  0x01
#define PNG_DISPOSE_OP_PREVIOUS    0x02

/* blend_op flags from inside fcTL */
#define PNG_BLEND_OP_SOURCE        0x00
#define PNG_BLEND_OP_OVER          0x01
#endif /* !PNG_APNG_SUPPORTED */

/* APNG typedefs that might not be present if APNG isn't supported. */

#ifndef PNG_APNG_SUPPORTED
typedef PNG_CALLBACK(void, *png_progressive_frame_ptr, (png_structp,
    png_uint_32));
typedef PNG_CALLBACK(void, *png_progressive_row_ptr, (png_structp, png_bytep,
    png_uint_32, int));
#endif /* PNG_APNG_SUPPORTED */

#ifndef PNG_APNG_SUPPORTED
#define PNG_APNG_SUPPORTED
#endif
#ifndef PNG_READ_APNG_SUPPORTED
#define PNG_READ_APNG_SUPPORTED
#endif
#ifndef PNG_WRITE_APNG_SUPPORTED
#define PNG_WRITE_APNG_SUPPORTED
#endif
#endif /* USE_INTERNAL_PNG */

#if !defined(USE_INTERNAL_PNG) || defined(USE_INTERNAL_PNG_DLL)
/* Either system libpng is used, or we're using our  *
 * libpng compiled as a DLL. The APNG function names *
 * will be defined as function pointers.             */
#ifdef PNG_EXPORT
#undef PNG_EXPORT
#endif
#define PNG_EXPORT(ordinal, type, name, args) \
	typedef type (*APNG_##name## _t) args; \
	extern APNG_##name##_t APNG_##name;

// Copied directly from png.h + APNG.
#ifdef PNG_APNG_SUPPORTED
PNG_EXPORT(245, png_uint_32, png_get_acTL, (png_structp png_ptr,
   png_infop info_ptr, png_uint_32 *num_frames, png_uint_32 *num_plays));

PNG_EXPORT(246, png_uint_32, png_set_acTL, (png_structp png_ptr,
   png_infop info_ptr, png_uint_32 num_frames, png_uint_32 num_plays));

PNG_EXPORT(247, png_uint_32, png_get_num_frames, (png_structp png_ptr,
   png_infop info_ptr));

PNG_EXPORT(248, png_uint_32, png_get_num_plays, (png_structp png_ptr,
   png_infop info_ptr));

PNG_EXPORT(249, png_uint_32, png_get_next_frame_fcTL,
   (png_structp png_ptr, png_infop info_ptr, png_uint_32 *width,
   png_uint_32 *height, png_uint_32 *x_offset, png_uint_32 *y_offset,
   png_uint_16 *delay_num, png_uint_16 *delay_den, png_byte *dispose_op,
   png_byte *blend_op));

PNG_EXPORT(250, png_uint_32, png_set_next_frame_fcTL,
   (png_structp png_ptr, png_infop info_ptr, png_uint_32 width,
   png_uint_32 height, png_uint_32 x_offset, png_uint_32 y_offset,
   png_uint_16 delay_num, png_uint_16 delay_den, png_byte dispose_op,
   png_byte blend_op));

PNG_EXPORT(251, png_uint_32, png_get_next_frame_width,
   (png_structp png_ptr, png_infop info_ptr));
PNG_EXPORT(252, png_uint_32, png_get_next_frame_height,
   (png_structp png_ptr, png_infop info_ptr));
PNG_EXPORT(253, png_uint_32, png_get_next_frame_x_offset,
   (png_structp png_ptr, png_infop info_ptr));
PNG_EXPORT(254, png_uint_32, png_get_next_frame_y_offset,
   (png_structp png_ptr, png_infop info_ptr));
PNG_EXPORT(255, png_uint_16, png_get_next_frame_delay_num,
   (png_structp png_ptr, png_infop info_ptr));
PNG_EXPORT(256, png_uint_16, png_get_next_frame_delay_den,
   (png_structp png_ptr, png_infop info_ptr));
PNG_EXPORT(257, png_byte, png_get_next_frame_dispose_op,
   (png_structp png_ptr, png_infop info_ptr));
PNG_EXPORT(258, png_byte, png_get_next_frame_blend_op,
   (png_structp png_ptr, png_infop info_ptr));
PNG_EXPORT(259, png_byte, png_get_first_frame_is_hidden,
   (png_structp png_ptr, png_infop info_ptr));
PNG_EXPORT(260, png_uint_32, png_set_first_frame_is_hidden,
   (png_structp png_ptr, png_infop info_ptr, png_byte is_hidden));

#ifdef PNG_READ_APNG_SUPPORTED
PNG_EXPORT(261, void, png_read_frame_head, (png_structp png_ptr,
   png_infop info_ptr));
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
PNG_EXPORT(262, void, png_set_progressive_frame_fn, (png_structp png_ptr,
   png_progressive_frame_ptr frame_info_fn,
   png_progressive_frame_ptr frame_end_fn));
#endif /* PNG_PROGRESSIVE_READ_SUPPORTED */
#endif /* PNG_READ_APNG_SUPPORTED */

#ifdef PNG_WRITE_APNG_SUPPORTED
PNG_EXPORT(263, void, png_write_frame_head, (png_structp png_ptr,
   png_infop info_ptr, png_bytepp row_pointers,
   png_uint_32 width, png_uint_32 height,
   png_uint_32 x_offset, png_uint_32 y_offset,
   png_uint_16 delay_num, png_uint_16 delay_den, png_byte dispose_op,
   png_byte blend_op));

PNG_EXPORT(264, void, png_write_frame_tail, (png_structp png_ptr,
   png_infop info_ptr));
#endif /* PNG_WRITE_APNG_SUPPORTED */
#endif /* PNG_APNG_SUPPORTED */

// Redefine the APNG function names.
#define png_get_acTL(png_ptr, info_ptr, num_frames, num_plays) \
	APNG_png_get_acTL(png_ptr, info_ptr, num_frames, num_plays)
#define png_set_acTL(png_ptr, info_ptr, num_frames, num_plays) \
	APNG_png_set_acTL(png_ptr, info_ptr, num_frames, num_plays)
#define png_get_num_frames(png_ptr, info_ptr) \
	APNG_png_get_num_frames(png_ptr, info_ptr)
#define png_get_num_plays(png_ptr, info_ptr) \
	APNG_png_get_num_plays(png_ptr, info_ptr)
#define png_get_next_frame_fcTL(png_ptr, info_ptr, width, height, x_offset, y_offset, delay_num, delay_den, dispose_op, blend_op) \
	APNG_png_get_next_frame_fcTL(png_ptr, info_ptr, width, height, x_offset, y_offset, delay_num, delay_den, dispose_op, blend_op)
#define png_set_next_frame_fcTL(png_ptr, info_ptr, width, height, x_offset, y_offset, delay_num, delay_den, dispose_op, blend_op) \
	APNG_png_set_next_frame_fcTL(png_ptr, info_ptr, width, height, x_offset, y_offset, delay_num, delay_den, dispose_op, blend_op)
#define png_get_next_frame_width(png_ptr, info_ptr) \
	APNG_png_get_next_frame_width(png_ptr, info_ptr)
#define png_get_next_frame_height(png_ptr, info_ptr) \
	APNG_png_get_next_frame_height(png_ptr, info_ptr)
#define png_get_next_frame_x_offset(png_ptr, info_ptr) \
	APNG_png_get_next_frame_x_offset(png_ptr, info_ptr)
#define png_get_next_frame_y_offset(png_ptr, info_ptr) \
	png_get_next_frame_y_offset(png_ptr, info_ptr)
#define png_get_next_frame_delay_num(png_ptr, info_ptr) \
	APNG_png_get_next_frame_delay_num(png_ptr, info_ptr)
#define png_get_next_frame_delay_den(png_ptr, info_ptr) \
	APNG_png_get_next_frame_delay_den(png_ptr, info_ptr)
#define png_get_next_frame_dispose_op(png_ptr, info_ptr) \
	APNG_png_get_next_frame_dispose_op(png_ptr, info_ptr)
#define png_get_next_frame_blend_op(png_ptr, info_ptr) \
	APNG_png_get_next_frame_blend_op(png_ptr, info_ptr)
#define png_get_first_frame_is_hidden(png_ptr, info_ptr) \
	APNG_png_get_first_frame_is_hidden(png_ptr, info_ptr)
#define png_set_first_frame_is_hidden(png_ptr, info_ptr, is_hidden) \
	APNG_png_set_first_frame_is_hidden(png_ptr, info_ptr, is_hidden)

#ifdef PNG_READ_APNG_SUPPORTED
#define png_read_frame_head(png_ptr, info_ptr) \
	APNG_png_read_frame_head(png_ptr, info_ptr)
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
#define png_set_progressive_frame_fn(png_ptr, frame_info_fn, frame_end_fn) \
	APNG_png_set_progressive_frame_fn(png_ptr, frame_info_fn, frame_end_fn)
#endif /* PNG_PROGRESSIVE_READ_SUPPORTED */
#endif /* PNG_READ_APNG_SUPPORTED */

#ifdef PNG_WRITE_APNG_SUPPORTED
#define png_write_frame_head(png_ptr, info_ptr, row_pointers, width, height, x_offset, y_offset, delay_num, delay_den, dispose_op, blend_op) \
	APNG_png_write_frame_head(png_ptr, info_ptr, row_pointers, width, height, x_offset, y_offset, delay_num, delay_den, dispose_op, blend_op)
#define png_write_frame_tail(png_ptr, info_ptr) \
	APNG_png_write_frame_tail(png_ptr, info_ptr)
#endif /* PNG_WRITE_APNG_SUPPORTED */

#endif /* USE_INTERNAL_PNG && !USE_INTERNAL_PNG_DLL */

#ifdef __cplusplus
}
#endif
