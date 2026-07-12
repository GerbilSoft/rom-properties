/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * mz_stream_IRpFile.hpp: IRpFile mz_stream implementation.                *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// librpfile
#include "librpfile/IRpFile.hpp"

namespace LibRomData { namespace mz_stream_IRpFile {

/**
 * Open a ZIP file for reading using an IRpFile.
 * @param stream
 * @param path Actually a pointer to an IRpFilePtr object
 * @param mode
 * @return
 */
int32_t mz_stream_IRpFile_open(void *stream, const char *path, int32_t mode);

/**
 * Open a ZIP file for reading using an IRpFile.
 * @param stream
 * @param path IRpFilePtr
 * @param mode
 * @return
 */
int32_t mz_stream_IRpFile_open(void *stream, const LibRpFile::IRpFilePtr &file, int32_t mode);

/**
 * Close an mz_stream_IRpFile.
 * @param stream
 * @return
 */
int32_t mz_stream_IRpFile_close(void *stream);

int32_t mz_stream_IRpFile_is_open(void *stream);
int32_t mz_stream_IRpFile_read(void *stream, void *buf, int32_t size);
int32_t mz_stream_IRpFile_write(void *stream, const void *buf, int32_t size);
int64_t mz_stream_IRpFile_tell(void *stream);
int32_t mz_stream_IRpFile_seek(void *stream, int64_t offset, int32_t origin);
int32_t mz_stream_IRpFile_close(void *stream);
int32_t mz_stream_IRpFile_error(void *stream);

/**
 * Create an mz_stream_IRpFile.
 * @return mz_stream_IRpFile
 */
void *mz_stream_IRpFile_create(void);

/**
 * Delete an mz_stream_IRpFile.
 * @param stream Pointer to mz_stream_IRpFile pointer
 */
void mz_stream_IRpFile_delete(void **stream);

} } // namespace LibRomData::mz_stream_IRpFile
