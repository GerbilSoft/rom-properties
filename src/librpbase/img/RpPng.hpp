/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpPng.hpp: PNG image handler.                                           *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C++ includes
#include <memory>

namespace LibRpFile {
	class IRpFile;
}
namespace LibRpTexture {
	class rp_image;
}

namespace LibRpBase {

struct IconAnimData;

namespace RpPng {

/**
 * Load a PNG image from an IRpFile.
 * @param file IRpFile to load from
 * @return rp_image*, or nullptr on error
 */
RP_LIBROMDATA_PUBLIC
std::shared_ptr<LibRpTexture::rp_image> load(const std::shared_ptr<LibRpFile::IRpFile> &file);

/**
 * Save an image in PNG format to an IRpFile.
 * IRpFile must be open for writing.
 *
 * NOTE: If the write fails, the caller will need
 * to delete the file.
 *
 * @param file IRpFile to write to
 * @param img rp_image to save
 * @return 0 on success; negative POSIX error code on error
 */
int save(const std::shared_ptr<LibRpFile::IRpFile> &file, const std::shared_ptr<const LibRpTexture::rp_image> &img);

/**
 * Save an image in PNG format to a file.
 *
 * @param filename Destination filename (UTF-8)
 * @param img rp_image to save
 * @return 0 on success; negative POSIX error code on error
 */
RP_LIBROMDATA_PUBLIC
int save(const char *filename, const std::shared_ptr<const LibRpTexture::rp_image> &img);

#ifdef _WIN32
/**
 * Save an image in PNG format to a file.
 *
 * @param filename Destination filename
 * @param img rp_image to save (UTF-16)
 * @return 0 on success; negative POSIX error code on error
 */
RP_LIBROMDATA_PUBLIC
int save(const wchar_t *filename, const std::shared_ptr<const LibRpTexture::rp_image> &img);
#endif /* _WIN32 */

/**
 * Save an animated image in APNG format to an IRpFile.
 * IRpFile must be open for writing.
 *
 * If the animated image contains a single frame,
 * a standard PNG image will be written.
 *
 * NOTE: If the image has multiple frames and APNG
 * write support is unavailable, -ENOTSUP will be
 * returned. The caller should then save the image
 * as a standard PNG file.
 *
 * NOTE 2: If the write fails, the caller will need
 * to delete the file.
 *
 * @param file IRpFile to write to
 * @param iconAnimData Animated image data to save
 * @return 0 on success; negative POSIX error code on error
 */
int save(const std::shared_ptr<LibRpFile::IRpFile> &file, const IconAnimData *iconAnimData);

/**
 * Save an animated image in APNG format to a file.
 *
 * If the animated image contains a single frame,
 * a standard PNG image will be written.
 *
 * NOTE: If the image has multiple frames and APNG
 * write support is unavailable, -ENOTSUP will be
 * returned. The caller should then save the image
 * as a standard PNG file.
 *
 * @param filename Destination filename (UTF-8)
 * @param iconAnimData Animated image data to save
 * @return 0 on success; negative POSIX error code on error
 */
RP_LIBROMDATA_PUBLIC
int save(const char *filename, const IconAnimData *iconAnimData);

#ifdef _WIN32
/**
 * Save an animated image in APNG format to a file.
 *
 * If the animated image contains a single frame,
 * a standard PNG image will be written.
 *
 * NOTE: If the image has multiple frames and APNG
 * write support is unavailable, -ENOTSUP will be
 * returned. The caller should then save the image
 * as a standard PNG file.
 *
 * @param filename Destination filename (UTF-16)
 * @param iconAnimData Animated image data to save
 * @return 0 on success; negative POSIX error code on error
 */
RP_LIBROMDATA_PUBLIC
int save(const wchar_t *filename, const IconAnimData *iconAnimData);
#endif /* _WIN32 */

/** Version info wrapper functions **/

/**
 * Was rom-properties compiled using zlib-ng?
 * @return True if zlib-ng; false if not.
 */
RP_LIBROMDATA_PUBLIC
bool zlib_is_ng(void);

/**
 * Get the zlib version string.
 * This is the runtime zlib version.
 * @return Result from zlibVersion() or zlibng_version().
 */
RP_LIBROMDATA_PUBLIC
const char *zlib_version_string(void);

/**
 * Does our libpng have APNG support?
 * @return True if APNG is supported; false if not.
 */
RP_LIBROMDATA_PUBLIC
bool libpng_has_APNG(void);

/**
 * Get the libpng version number.
 * This is the runtime libpng version.
 * @return Result from png_access_version_number()
 */
RP_LIBROMDATA_PUBLIC
uint32_t libpng_version_number(void);

/**
 * Get the libpng copyright string.
 * @return libpng copyright string [png_get_copyright()]
 */
RP_LIBROMDATA_PUBLIC
const char *libpng_copyright_string(void);

} }
