/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * sixel-mini.h: Subset of definitions from libsixel's headers.            *
 *                                                                         *
 * Copyright (c) 2021 libsixel developers. See `AUTHORS-sixel`.            *
 * Copyright (c) 2014-2020 Hayaki Saito                                    *
 * SPDX-License-Identifier: MIT                                            *
 ***************************************************************************/

// from libsixel-1.10.5
// (should be compatible with libsixel-1.8.6)

#pragma once

#include <stddef.h>  /* for size_t */

// Not using SIXELAPI because we're using dlopen() / GetProcAddress().
#define SIXELAPI

#ifdef __cplusplus
extern "C" {
#endif

/* return value */
typedef int SIXELSTATUS;
#define SIXEL_OK                0x0000                          /* succeeded */
#define SIXEL_FALSE             0x1000                          /* failed */

#define SIXEL_RUNTIME_ERROR     (SIXEL_FALSE         | 0x0100)  /* runtime error */
#define SIXEL_LOGIC_ERROR       (SIXEL_FALSE         | 0x0200)  /* logic error */
#define SIXEL_FEATURE_ERROR     (SIXEL_FALSE         | 0x0300)  /* feature not enabled */
#define SIXEL_LIBC_ERROR        (SIXEL_FALSE         | 0x0400)  /* errors caused by curl */
#define SIXEL_CURL_ERROR        (SIXEL_FALSE         | 0x0500)  /* errors occures in libc functions */
#define SIXEL_JPEG_ERROR        (SIXEL_FALSE         | 0x0600)  /* errors occures in libjpeg functions */
#define SIXEL_PNG_ERROR         (SIXEL_FALSE         | 0x0700)  /* errors occures in libpng functions */
#define SIXEL_GDK_ERROR         (SIXEL_FALSE         | 0x0800)  /* errors occures in gdk functions */
#define SIXEL_GD_ERROR          (SIXEL_FALSE         | 0x0900)  /* errors occures in gd functions */
#define SIXEL_STBI_ERROR        (SIXEL_FALSE         | 0x0a00)  /* errors occures in stb_image functions */
#define SIXEL_STBIW_ERROR       (SIXEL_FALSE         | 0x0b00)  /* errors occures in stb_image_write functions */

#define SIXEL_INTERRUPTED       (SIXEL_OK            | 0x0001)  /* interrupted by a signal */

#define SIXEL_BAD_ALLOCATION    (SIXEL_RUNTIME_ERROR | 0x0001)  /* malloc() failed */
#define SIXEL_BAD_ARGUMENT      (SIXEL_RUNTIME_ERROR | 0x0002)  /* bad argument detected */
#define SIXEL_BAD_INPUT         (SIXEL_RUNTIME_ERROR | 0x0003)  /* bad input detected */
#define SIXEL_BAD_INTEGER_OVERFLOW (SIXEL_RUNTIME_ERROR | 0x0004)  /* integer overflow */

#define SIXEL_NOT_IMPLEMENTED   (SIXEL_FEATURE_ERROR | 0x0001)  /* feature not implemented */

#define SIXEL_SUCCEEDED(status) (((status) & 0x1000) == 0)
#define SIXEL_FAILED(status)    (((status) & 0x1000) != 0)

/* method for finding the largest dimension for splitting,
 * and sorting by that component */
#define SIXEL_LARGE_AUTO   0x0   /* choose automatically the method for finding the largest
                                    dimension */
#define SIXEL_LARGE_NORM   0x1   /* simply comparing the range in RGB space */
#define SIXEL_LARGE_LUM    0x2   /* transforming into luminosities before the comparison */

/* method for choosing a color from the box */
#define SIXEL_REP_AUTO            0x0  /* choose automatically the method for selecting
                                          representative color from each box */
#define SIXEL_REP_CENTER_BOX      0x1  /* choose the center of the box */
#define SIXEL_REP_AVERAGE_COLORS  0x2  /* choose the average all the color
                                          in the box (specified in Heckbert's paper) */
#define SIXEL_REP_AVERAGE_PIXELS  0x3  /* choose the average all the pixels in the box */

/* method for diffusing */
#define SIXEL_DIFFUSE_AUTO        0x0  /* choose diffusion type automatically */
#define SIXEL_DIFFUSE_NONE        0x1  /* don't diffuse */
#define SIXEL_DIFFUSE_ATKINSON    0x2  /* diffuse with Bill Atkinson's method */
#define SIXEL_DIFFUSE_FS          0x3  /* diffuse with Floyd-Steinberg method */
#define SIXEL_DIFFUSE_JAJUNI      0x4  /* diffuse with Jarvis, Judice & Ninke method */
#define SIXEL_DIFFUSE_STUCKI      0x5  /* diffuse with Stucki's method */
#define SIXEL_DIFFUSE_BURKES      0x6  /* diffuse with Burkes' method */
#define SIXEL_DIFFUSE_A_DITHER    0x7  /* positionally stable arithmetic dither */
#define SIXEL_DIFFUSE_X_DITHER    0x8  /* positionally stable arithmetic xor based dither */

/* quality modes */
#define SIXEL_QUALITY_AUTO        0x0  /* choose quality mode automatically */
#define SIXEL_QUALITY_HIGH        0x1  /* high quality palette construction */
#define SIXEL_QUALITY_LOW         0x2  /* low quality palette construction */
#define SIXEL_QUALITY_FULL        0x3  /* full quality palette construction */
#define SIXEL_QUALITY_HIGHCOLOR   0x4  /* high color */

/* built-in dither */
#define SIXEL_BUILTIN_MONO_DARK   0x0  /* monochrome terminal with dark background */
#define SIXEL_BUILTIN_MONO_LIGHT  0x1  /* monochrome terminal with light background */
#define SIXEL_BUILTIN_XTERM16     0x2  /* xterm 16color */
#define SIXEL_BUILTIN_XTERM256    0x3  /* xterm 256color */
#define SIXEL_BUILTIN_VT340_MONO  0x4  /* vt340 monochrome */
#define SIXEL_BUILTIN_VT340_COLOR 0x5  /* vt340 color */
#define SIXEL_BUILTIN_G1          0x6  /* 1bit grayscale */
#define SIXEL_BUILTIN_G2          0x7  /* 2bit grayscale */
#define SIXEL_BUILTIN_G4          0x8  /* 4bit grayscale */
#define SIXEL_BUILTIN_G8          0x9  /* 8bit grayscale */

/* offset value of pixelFormat */
#define SIXEL_FORMATTYPE_COLOR     (0)
#define SIXEL_FORMATTYPE_GRAYSCALE (1 << 6)
#define SIXEL_FORMATTYPE_PALETTE   (1 << 7)

/* pixelformat type of input image
   NOTE: for compatibility, the value of PIXELFORAMT_COLOR_RGB888 must be 3 */
#define SIXEL_PIXELFORMAT_RGB555   (SIXEL_FORMATTYPE_COLOR     | 0x01) /* 15bpp */
#define SIXEL_PIXELFORMAT_RGB565   (SIXEL_FORMATTYPE_COLOR     | 0x02) /* 16bpp */
#define SIXEL_PIXELFORMAT_RGB888   (SIXEL_FORMATTYPE_COLOR     | 0x03) /* 24bpp */
#define SIXEL_PIXELFORMAT_BGR555   (SIXEL_FORMATTYPE_COLOR     | 0x04) /* 15bpp */
#define SIXEL_PIXELFORMAT_BGR565   (SIXEL_FORMATTYPE_COLOR     | 0x05) /* 16bpp */
#define SIXEL_PIXELFORMAT_BGR888   (SIXEL_FORMATTYPE_COLOR     | 0x06) /* 24bpp */
#define SIXEL_PIXELFORMAT_ARGB8888 (SIXEL_FORMATTYPE_COLOR     | 0x10) /* 32bpp */
#define SIXEL_PIXELFORMAT_RGBA8888 (SIXEL_FORMATTYPE_COLOR     | 0x11) /* 32bpp */
#define SIXEL_PIXELFORMAT_ABGR8888 (SIXEL_FORMATTYPE_COLOR     | 0x12) /* 32bpp */
#define SIXEL_PIXELFORMAT_BGRA8888 (SIXEL_FORMATTYPE_COLOR     | 0x13) /* 32bpp */
#define SIXEL_PIXELFORMAT_G1       (SIXEL_FORMATTYPE_GRAYSCALE | 0x00) /* 1bpp grayscale */
#define SIXEL_PIXELFORMAT_G2       (SIXEL_FORMATTYPE_GRAYSCALE | 0x01) /* 2bpp grayscale */
#define SIXEL_PIXELFORMAT_G4       (SIXEL_FORMATTYPE_GRAYSCALE | 0x02) /* 4bpp grayscale */
#define SIXEL_PIXELFORMAT_G8       (SIXEL_FORMATTYPE_GRAYSCALE | 0x03) /* 8bpp grayscale */
#define SIXEL_PIXELFORMAT_AG88     (SIXEL_FORMATTYPE_GRAYSCALE | 0x13) /* 16bpp gray+alpha */
#define SIXEL_PIXELFORMAT_GA88     (SIXEL_FORMATTYPE_GRAYSCALE | 0x23) /* 16bpp gray+alpha */
#define SIXEL_PIXELFORMAT_PAL1     (SIXEL_FORMATTYPE_PALETTE   | 0x00) /* 1bpp palette */
#define SIXEL_PIXELFORMAT_PAL2     (SIXEL_FORMATTYPE_PALETTE   | 0x01) /* 2bpp palette */
#define SIXEL_PIXELFORMAT_PAL4     (SIXEL_FORMATTYPE_PALETTE   | 0x02) /* 4bpp palette */
#define SIXEL_PIXELFORMAT_PAL8     (SIXEL_FORMATTYPE_PALETTE   | 0x03) /* 8bpp palette */

/* palette type */
#define SIXEL_PALETTETYPE_AUTO     0   /* choose palette type automatically */
#define SIXEL_PALETTETYPE_HLS      1   /* HLS colorspace */
#define SIXEL_PALETTETYPE_RGB      2   /* RGB colorspace */

/* policies of SIXEL encoding */
#define SIXEL_ENCODEPOLICY_AUTO    0   /* choose encoding policy automatically */
#define SIXEL_ENCODEPOLICY_FAST    1   /* encode as fast as possible */
#define SIXEL_ENCODEPOLICY_SIZE    2   /* encode to as small sixel sequence as possible */

/* method for re-sampling */
#define SIXEL_RES_NEAREST          0   /* Use nearest neighbor method */
#define SIXEL_RES_GAUSSIAN         1   /* Use guaussian filter */
#define SIXEL_RES_HANNING          2   /* Use hanning filter */
#define SIXEL_RES_HAMMING          3   /* Use hamming filter */
#define SIXEL_RES_BILINEAR         4   /* Use bilinear filter */
#define SIXEL_RES_WELSH            5   /* Use welsh filter */
#define SIXEL_RES_BICUBIC          6   /* Use bicubic filter */
#define SIXEL_RES_LANCZOS2         7   /* Use lanczos-2 filter */
#define SIXEL_RES_LANCZOS3         8   /* Use lanczos-3 filter */
#define SIXEL_RES_LANCZOS4         9   /* Use lanczos-4 filter */

struct sixel_allocator;
typedef struct sixel_allocator sixel_allocator_t;

/* output context manipulation API */

struct sixel_output;
typedef struct sixel_output sixel_output_t;
typedef int (* sixel_write_function)(char *data, int size, void *priv);

/* create new output context object */
SIXELAPI SIXELSTATUS
sixel_output_new(
    sixel_output_t          /* out */ **output,     /* output object to be created */
    sixel_write_function    /* in */  fn_write,     /* callback for output sixel */
    void                    /* in */ *priv,         /* private data given as
                                                       3rd argument of fn_write */
    sixel_allocator_t       /* in */  *allocator);  /* allocator, null if you use
                                                       default allocator */

/* destroy output context object */
SIXELAPI void
sixel_output_destroy(sixel_output_t /* in */ *output); /* output context */

/* color quantization API */

/* handle type of dither context object */
struct sixel_dither;
typedef struct sixel_dither sixel_dither_t;

/* create dither context object */
SIXELAPI SIXELSTATUS
sixel_dither_new(
    sixel_dither_t      /* out */   **ppdither,  /* dither object to be created */
    int                 /* in */    ncolors,     /* required colors */
    sixel_allocator_t   /* in */    *allocator); /* allocator, null if you use
                                                    default allocator */

/* destroy dither context object */
SIXELAPI void
sixel_dither_destroy(sixel_dither_t *dither); /* dither context object */

/* initialize internal palette from specified pixel buffer */
SIXELAPI SIXELSTATUS
sixel_dither_initialize(
    sixel_dither_t *dither,                    /* dither context object */
    unsigned char /* in */ *data,              /* sample image */
    int           /* in */ width,              /* image width */
    int           /* in */ height,             /* image height */
    int           /* in */ pixelformat,        /* one of enum pixelFormat */
    int           /* in */ method_for_largest, /* method for finding the largest dimension */
    int           /* in */ method_for_rep,     /* method for choosing a color from the box */
    int           /* in */ quality_mode);      /* quality of histogram processing */

/* set palette */
SIXELAPI void
sixel_dither_set_palette(
    sixel_dither_t /* in */ *dither,   /* dither context object */
    unsigned char  /* in */ *palette);

/* set pixelformat */
SIXELAPI void
sixel_dither_set_pixelformat(
    sixel_dither_t /* in */ *dither,      /* dither context object */
    int            /* in */ pixelformat); /* one of enum pixelFormat */

/* converter API */

/* convert pixels into sixel format and write it to output context */
SIXELAPI SIXELSTATUS
sixel_encode(
    unsigned char  /* in */ *pixels,     /* pixel bytes */
    int            /* in */  width,      /* image width */
    int            /* in */  height,     /* image height */
    int            /* in */  depth,      /* color depth: now unused */
    sixel_dither_t /* in */ *dither,     /* dither context */
    sixel_output_t /* in */ *context);   /* output context */

#ifdef __cplusplus
}
#endif
