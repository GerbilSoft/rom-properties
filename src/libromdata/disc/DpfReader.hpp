/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DpfReader.hpp: GameCube/Wii DPF/RPF sparse disc image reader.           *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: This class does NOT derive from SparseDiscReader because
// DPF/RPF uses byte-granularity, not block-granularity.

#pragma once

#include "librpbase/disc/IDiscReader.hpp"

namespace LibRomData {

class DpfReaderPrivate;
class DpfReader final : public LibRpBase::IDiscReader
{
public:
	/**
	 * Construct a DpfReader with the specified file.
	 * The file is ref()'d, so the original file can be
	 * unref()'d by the caller afterwards.
	 * @param file File to read from.
	 */
	explicit DpfReader(const LibRpFile::IRpFilePtr &file);
public:
	~DpfReader() final;

private:
	typedef IDiscReader super;
	RP_DISABLE_COPY(DpfReader)
private:
	friend class DpfReaderPrivate;
	DpfReaderPrivate *const d_ptr;

public:
	/** Disc image detection functions **/

	/**
	 * Is a disc image supported by this class?
	 * @param pHeader Disc image header.
	 * @param szHeader Size of header.
	 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
	 */
	ATTR_ACCESS_SIZE(read_only, 1, 2)
	static int isDiscSupported_static(const uint8_t *pHeader, size_t szHeader);

	/**
	 * Is a disc image supported by this object?
	 * @param pHeader Disc image header.
	 * @param szHeader Size of header.
	 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	int isDiscSupported(const uint8_t *pHeader, size_t szHeader) const final;

public:
	/** IDiscReader functions **/

	/**
	 * Read data from the disc image.
	 * @param ptr Output data buffer.
	 * @param size Amount of data to read, in bytes.
	 * @return Number of bytes read.
	 */
	ATTR_ACCESS_SIZE(write_only, 2, 3)
	size_t read(void *ptr, size_t size) final;

	/**
	 * Set the disc image position.
	 * @param pos disc image position.
	 * @return 0 on success; -1 on error.
	 */
	int seek(off64_t pos) final;

	/**
	 * Get the disc image position.
	 * @return Disc image position on success; -1 on error.
	 */
	off64_t tell(void) final;

	/**
	 * Get the disc image size.
	 * @return Disc image size, or -1 on error.
	 */
	off64_t size(void) final;
};

}
