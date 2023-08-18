/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RelatedFile.hpp: Open a related file.                                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C++ includes
#include <memory>

namespace LibRpFile {
	class IRpFile;
}

namespace LibRpFile { namespace FileSystem {

/**
 * Attempt to open a related file. (read-only)
 * RAW POINTER VERSION; use with caution.
 *
 * Related files are located in the same directory as the
 * primary file, but may have a different filename and/or
 * file extension.
 *
 * If the primary file is a symlink, the related file may
 * be located in the original file's directory.
 *
 * @param filename	[in] Primary filename. (UTF-8)
 * @param basename	[in,opt] New basename. (UTF-8) If nullptr, uses the existing basename.
 * @param ext		[in] New extension, including leading dot. (UTF-8)
 * @return IRpFile*, or nullptr if not found.
 */
LibRpFile::IRpFile *openRelatedFile_rawptr(const char *filename, const char *basename, const char *ext);

/**
 * Attempt to open a related file. (read-only)
 *
 * Related files are located in the same directory as the
 * primary file, but may have a different filename and/or
 * file extension.
 *
 * If the primary file is a symlink, the related file may
 * be located in the original file's directory.
 *
 * @param filename	[in] Primary filename. (UTF-8)
 * @param basename	[in,opt] New basename. (UTF-8) If nullptr, uses the existing basename.
 * @param ext		[in] New extension, including leading dot. (UTF-8)
 * @return shared_ptr<IRpFile>, or nullptr if not found.
 */
std::shared_ptr<LibRpFile::IRpFile> openRelatedFile(const char *filename, const char *basename, const char *ext);

#ifdef _WIN32
/**
 * Attempt to open a related file. (read-only)
 * RAW POINTER VERSION; use with caution.
 *
 * Related files are located in the same directory as the
 * primary file, but may have a different filename and/or
 * file extension.
 *
 * If the primary file is a symlink, the related file may
 * be located in the original file's directory.
 *
 * @param filename	[in] Primary filename. (UTF-16)
 * @param basename	[in,opt] New basename. (UTF-16) If nullptr, uses the existing basename.
 * @param ext		[in] New extension, including leading dot. (UTF-16)
 * @return IRpFile*, or nullptr if not found.
 */
LibRpFile::IRpFile *openRelatedFile_rawptr(const wchar_t *filenameW, const wchar_t *basenameW, const wchar_t *extW);

/**
 * Attempt to open a related file. (read-only)
 *
 * Related files are located in the same directory as the
 * primary file, but may have a different filename and/or
 * file extension.
 *
 * If the primary file is a symlink, the related file may
 * be located in the original file's directory.
 *
 * @param filename	[in] Primary filename. (UTF-16)
 * @param basename	[in,opt] New basename. (UTF-16) If nullptr, uses the existing basename.
 * @param ext		[in] New extension, including leading dot. (UTF-16)
 * @return shared_ptr<IRpFile>, or nullptr if not found.
 */
std::shared_ptr<LibRpFile::IRpFile> openRelatedFile(const wchar_t *filenameW, const wchar_t *basenameW, const wchar_t *extW);
#endif /* _WIN32 */

} }
