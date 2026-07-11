/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * mz_stream_IRpFile.cpp: IRpFile filefuncs for MiniZip-NG.                *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpbase.h"
#include "mz_stream_IRpFile.hpp"

// MiniZip-NG
#include "mz.h"
#include "mz_strm.h"

// librpfile
using LibRpFile::IRpFile;
using LibRpFile::IRpFilePtr;

namespace LibRomData { namespace mz_stream_IRpFile {

// NOTE: Due to the way mz_stream works compared to the compat API,
// we don't need to actually call any MiniZip-NG functions here,
// so the Delay-Load checks have been removed.

static const mz_stream_vtbl mz_stream_IRpFile_vtbl = {
	mz_stream_IRpFile_open,
	mz_stream_IRpFile_is_open,
	mz_stream_IRpFile_read,
	mz_stream_IRpFile_write,
	mz_stream_IRpFile_tell,
	mz_stream_IRpFile_seek,
	mz_stream_IRpFile_close,
	mz_stream_IRpFile_error,
	mz_stream_IRpFile_create,
	mz_stream_IRpFile_delete,

	nullptr,	// get_prop_int64
	nullptr,	// set_prop_int64
};

struct mz_stream_IRpFile_t {
	mz_stream stream;
	IRpFilePtr file;
};

/**
 * Open a ZIP file for reading using an IRpFile.
 * @param stream
 * @param path Actually a pointer to an IRpFilePtr object
 * @param mode
 * @return
 */
int32_t mz_stream_IRpFile_open(void *stream, const char *path, int32_t mode)
{
	// `path` should actually be a pointer to an IRpFilePtr.
	return mz_stream_IRpFile_open(stream, *(reinterpret_cast<const IRpFilePtr*>(path)), mode);
}

/**
 * Open a ZIP file for reading using an IRpFile.
 * @param stream
 * @param path IRpFilePtr
 * @param mode
 * @return
 */
int32_t mz_stream_IRpFile_open(void *stream, const IRpFilePtr &file, int32_t mode)
{
	// NOTE: Ignoring mode for now...
	// TODO: Check if mode matches the IRpFile mode?
	RP_UNUSED(mode);

	if (!file) {
		return MZ_OPEN_ERROR;
	}

	mz_stream_IRpFile_t *const s = static_cast<mz_stream_IRpFile_t*>(stream);
	s->file = file;
	return MZ_OK;
}

int32_t mz_stream_IRpFile_is_open(void *stream)
{
	const mz_stream_IRpFile_t *const s = static_cast<mz_stream_IRpFile_t*>(stream);
	return (bool)s->file ? MZ_OK : MZ_OPEN_ERROR;
}

int32_t mz_stream_IRpFile_read(void *stream, void *buf, int32_t size)
{
	const mz_stream_IRpFile_t *const s = static_cast<mz_stream_IRpFile_t*>(stream);
	return static_cast<int32_t>(s->file->read(buf, size));
}

int32_t mz_stream_IRpFile_write(void *stream, const void *buf, int32_t size)
{
	const mz_stream_IRpFile_t *const s = static_cast<mz_stream_IRpFile_t*>(stream);
	return static_cast<int32_t>(s->file->write(buf, size));
}

int64_t mz_stream_IRpFile_tell(void *stream)
{
	const mz_stream_IRpFile_t *const s = static_cast<mz_stream_IRpFile_t*>(stream);
	return s->file->tell();
}

int32_t mz_stream_IRpFile_seek(void *stream, int64_t offset, int32_t origin)
{
	const mz_stream_IRpFile_t *const s = static_cast<mz_stream_IRpFile_t*>(stream);

	// MiniZip-NG's `origin` value matches IRpFile::SeekWhence.
	// We can use a static_cast<> to convert it for file->seek().
	return s->file->seek(offset, static_cast<IRpFile::SeekWhence>(origin));
}

int32_t mz_stream_IRpFile_close(void *stream)
{
	// close() resets the IRpFilePtr.
	// The file might still be open if there are still references to it.
	mz_stream_IRpFile_t *const s = static_cast<mz_stream_IRpFile_t*>(stream);
	s->file.reset();
	return MZ_OK;
}

int32_t mz_stream_IRpFile_error(void *stream)
{
	// TODO: Get the actual error.
	RP_UNUSED(stream);
	return MZ_OK;
}

/**
 * Create an mz_stream_IRpFile.
 * @return mz_stream_IRpFile
 */
void *mz_stream_IRpFile_create(void)
{
	// NOTE: Need to use new due to IRpFilePtr.
	mz_stream_IRpFile_t *const s = new mz_stream_IRpFile_t();
	if (s) {
		// FIXME: MiniZip-NG's stream vtbl pointer is *not* const...
		s->stream.vtbl = const_cast<mz_stream_vtbl*>(&mz_stream_IRpFile_vtbl);
		s->stream.base = nullptr;
	}
	return s;
}

/**
 * Delete an mz_stream_IRpFile.
 * @param stream Pointer to mz_stream_IRpFile pointer
 */
void mz_stream_IRpFile_delete(void **stream)
{
	if (!stream) {
		return;
	}
	mz_stream_IRpFile_t *const s = static_cast<mz_stream_IRpFile_t*>(*stream);
	if (s) {
		delete s;
	}
	*stream = nullptr;
}

} } // namespace LibRomData::mz_stream_IRpFile
