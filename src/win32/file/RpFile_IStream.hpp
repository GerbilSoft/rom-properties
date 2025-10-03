/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RpFile_IStream.hpp: IRpFile using an IStream*.                          *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpfile/IRpFile.hpp"
#include <objidl.h>

// MinGW-w64's comdefsp.h only works properly with MSVC,
// since it uses __uuidof().
#ifndef _MSC_VER
_COM_SMARTPTR_TYPEDEF(IStream, IID_IStream);
#endif /* _MSC_VER */

// zlib
struct z_stream_s;

class RpFile_IStream final : public LibRpFile::IRpFile
{
public:
	/**
	 * Create an IRpFile using IStream* as the underlying storage mechanism.
	 * @param pStream	[in] IStream*.
	 * @param gzip		[in] If true, handle gzipped files automatically.
	 */
	explicit RpFile_IStream(IStream *pStream, bool gzip = false);
public:
	~RpFile_IStream() final;

private:
	typedef LibRpFile::IRpFile super;
	RP_DISABLE_COPY(RpFile_IStream)

public:
	/**
	 * Is the file open?
	 * This usually only returns false if an error occurred.
	 * @return True if the file is open; false if it isn't.
	 */
	bool isOpen(void) const final;

	/**
	 * Close the file.
	 */
	void close(void) final;

	/**
	 * Read data from the file.
	 * @param ptr Output data buffer.
	 * @param size Amount of data to read, in bytes.
	 * @return Number of bytes read.
	 */
	ATTR_ACCESS_SIZE(write_only, 2, 3)
	size_t read(void *ptr, size_t size) final;

	/**
	 * Write data to the file.
	 * @param ptr Input data buffer.
	 * @param size Amount of data to read, in bytes.
	 * @return Number of bytes written.
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	size_t write(const void *ptr, size_t size) final;

	/**
	 * Set the file position.
	 * @param pos		[in] File position
	 * @param whence	[in] Where to seek from
	 * @return 0 on success; -1 on error.
	 */
	int seek(off64_t pos, SeekWhence whence) final;

	/**
	 * Get the file position.
	 * @return File position, or -1 on error.
	 */
	off64_t tell(void) final;

	/**
	 * Truncate the file.
	 * @param size New size. (default is 0)
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int truncate(off64_t size = 0) final;

	/**
	 * Flush buffers.
	 * This operation only makes sense on writable files.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int flush(void) final;

public:
	/** File properties **/

	/**
	 * Get the file size.
	 * @return File size, or negative on error.
	 */
	off64_t size(void) final;

	/**
	 * Get the filename.
	 * @return Filename. (May be nullptr if the filename is not available.)
	 */
	const char *filename(void) const final;

public:
	/** Extra functions **/

	/**
	 * Make the file writable.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int makeWritable(void) final
	{
		// TODO: Actually do something here...
		// For now, return 0 if writable; -ENOTSUP if not.
		return (isWritable() ? 0 : -ENOTSUP);
	}

protected:
	IStreamPtr m_pStream;
	std::string m_filename;

	// zlib
	unsigned int m_z_uncomp_sz;
	unsigned int m_z_filepos;	// position in compressed file
	off64_t m_z_realpos;		// position in real file
	struct z_stream_s *m_pZstm;
	// zlib buffer
	uint8_t *m_pZbuf;
	ULONG m_zbufLen;
	ULONG m_zcurPos;

	/**
	 * Copy the zlib stream from another RpFile_IStream.
	 * @param other
	 * @return 0 on success; non-zero on error.
	 */
	int copyZlibStream(const RpFile_IStream &other);
};
