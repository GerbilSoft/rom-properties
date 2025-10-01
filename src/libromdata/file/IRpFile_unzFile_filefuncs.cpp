/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IRpFile_unzFile_filefuncs.cpp: IRpFile filefuncs for MiniZip-NG.        *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "config.librpbase.h"
#include "IRpFile_unzFile_filefuncs.hpp"

// MiniZip-NG
#include "mz.h"

// librpfile
using LibRpFile::IRpFilePtr;

#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRomData { namespace IRpFile_unzFile_filefuncs {

#ifdef _MSC_VER
// DelayLoad test implementations
#  ifdef ZLIB_IS_DLL
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#  endif /* ZLIB_IS_DLL */
#  ifdef MINIZIP_IS_DLL
// unzClose() can safely take nullptr; it won't do anything.
DELAYLOAD_TEST_FUNCTION_IMPL1(unzClose, nullptr);
#  endif /* MINIZIP_IS_DLL */
#endif /* _MSC_VER */

// NOTE: Only implementing the LFS (64-bit) functions.

static void* ZCALLBACK IRpFile_open64_file_func(void *opaque, const void *filename, int mode)
{
	// filename should actually be a pointer to an IRpFilePtr.
	// NOTE: Ignoring mode for now...
	// TODO: Check if mode matches the IRpFile mode?
	RP_UNUSED(opaque);
	RP_UNUSED(mode);

	// Returning a copy of the pointer to IRpFilePtr, as a pointer.
	// Can't return IRpFilePtr by itself due to using a C interface.
	return new IRpFilePtr(*(IRpFilePtr*)filename);
}

static unsigned long ZCALLBACK IRpFile_read_file_func(void *opaque, void *stream, void *buf, unsigned long size)
{
	RP_UNUSED(opaque);
	IRpFilePtr &file = *(reinterpret_cast<IRpFilePtr*>(stream));
	return static_cast<unsigned long>(file->read(buf, size));
}

static unsigned long ZCALLBACK IRpFile_write_file_func(void *opaque, void *stream, const void *buf, unsigned long size)
{
	RP_UNUSED(opaque);
	IRpFilePtr &file = *(reinterpret_cast<IRpFilePtr*>(stream));
	return static_cast<unsigned long>(file->write(buf, size));
}

static ZPOS64_T ZCALLBACK IRpFile_tell64_file_func(void *opaque, void *stream)
{
	RP_UNUSED(opaque);
	IRpFilePtr &file = *(reinterpret_cast<IRpFilePtr*>(stream));
	return file->tell();
}

static long ZCALLBACK IRpFile_seek64_file_func(void *opaque, void *stream, ZPOS64_T offset, int origin)
{
	RP_UNUSED(opaque);
	IRpFilePtr &file = *(reinterpret_cast<IRpFilePtr*>(stream));

	// NOTE: IRpFile doesn't support origin.
	// Emulate it here.
	off64_t pos = 0;
	switch (origin) {
		default:
		case MZ_SEEK_SET:
			break;
		case MZ_SEEK_CUR:
			pos = file->tell();
			break;
		case MZ_SEEK_END:
			pos = file->size();
			break;
	}

	return file->seek(pos + offset);
}

static int ZCALLBACK IRpFile_close_file_func(void *opaque, void *stream)
{
	// close() deletes the IRpFilePtr.
	// The file might still be open if there are still references to it.
	RP_UNUSED(opaque);
	delete reinterpret_cast<IRpFilePtr*>(stream);
	return MZ_OK;
}

static int ZCALLBACK IRpFile_testerror_file_func(void *opaque, void *stream)
{
	// TODO: Get the actual error.
	RP_UNUSED(opaque);
	RP_UNUSED(stream);
	return MZ_OK;
}

static const zlib_filefunc64_def IRpFile_filefunc_def = {
	IRpFile_open64_file_func,
	IRpFile_read_file_func,
	IRpFile_write_file_func,
	IRpFile_tell64_file_func,
	IRpFile_seek64_file_func,
	IRpFile_close_file_func,
	IRpFile_testerror_file_func,
	nullptr,
};

/**
 * Fill in filefuncs for IRpFile.
 * @param pzlib_filefunc_def
 */
void fill_IRpFile_filefunc64(zlib_filefunc64_def *pzlib_filefunc_def)
{
	*pzlib_filefunc_def = IRpFile_filefunc_def;
}

/**
 * Open a ZIP file for reading using an IRpFile.
 * @param file IRpFilePtr
 * @return unzFile, or nullptr on error.
 */
unzFile unzOpen2_64_IRpFile(const LibRpFile::IRpFilePtr &file)
{
#ifdef _MSC_VER
	// Delay load verification.
#  ifdef ZLIB_IS_DLL
	// Only if zlib is a DLL.
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		return nullptr;
	}
#  else /* !ZLIB_IS_DLL */
	// zlib isn't in a DLL, but we need to ensure that the
	// CRC table is initialized anyway.
	get_crc_table();
#  endif /* ZLIB_IS_DLL */

#  ifdef MINIZIP_IS_DLL
	// Only if MiniZip is a DLL.
	if (DelayLoad_test_unzClose() != 0) {
		// Delay load failed.
		return nullptr;
	}
#  endif /* MINIZIP_IS_DLL */
#endif /* _MSC_VER */

	// NOTE: MiniZip-NG doesn't write to the filefunc struct,
	// but it's not marked as const...
	return unzOpen2_64((void*)&file, const_cast<zlib_filefunc64_def*>(&IRpFile_filefunc_def));
}

} }
