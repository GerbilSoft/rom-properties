/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Z3DSReader.cpp: Nintendo 3DS Z3DS reader.                               *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "z3ds_structs.h"

// librpbase
#include "librpbase/disc/IDiscReader.hpp"
#include "librpbase/crypto/KeyManager.hpp"

// CIAReader
#include "CIAReader.hpp"

// C++ STL classes
#include <string>

namespace LibRomData {

class Z3DSReaderPrivate;
class Z3DSReader final : public LibRpBase::IDiscReader
{
public:
	/**
	 * Construct an Z3DSReader with the specified IRpFile.
	 * @param file IRpFile
	 */
	Z3DSReader(const LibRpFile::IRpFilePtr &file);
public:
	~Z3DSReader() final;

private:
	typedef IDiscReader super;
	RP_DISABLE_COPY(Z3DSReader)

protected:
	friend class Z3DSReaderPrivate;
	Z3DSReaderPrivate *const d_ptr;

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
	/** IDiscReader **/

	/**
	 * Read data from the partition.
	 * @param ptr Output data buffer.
	 * @param size Amount of data to read, in bytes.
	 * @return Number of bytes read.
	 */
	ATTR_ACCESS_SIZE(write_only, 2, 3)
	size_t read(void *ptr, size_t size) final;

	/**
	 * Set the partition position.
	 * @param pos		[in] Partition position
	 * @param whence	[in] Where to seek from
	 * @return 0 on success; -1 on error.
	 */
	int seek(off64_t pos, SeekWhence whence) final;

	/**
	 * Get the partition position.
	 * @return Partition position on success; -1 on error.
	 */
	off64_t tell(void) final;

	/**
	 * Get the data size.
	 * This size does not include the partition header,
	 * and it's adjusted to exclude hashes.
	 * @return Data size, or -1 on error.
	 */
	off64_t size(void) final;

public:
	/** Z3DSReader-specific functions **/

	/**
	 * Get the metadata.
	 * @return Metadata, or empty map if not present or an error occurred.
	 */
	std::vector<std::pair<std::string, std::vector<uint8_t>>> getZ3DSMetaData(void);
};

typedef std::shared_ptr<Z3DSReader> Z3DSReaderPtr;
typedef std::shared_ptr<Z3DSReader> Z3DSReaderConstPtr;

}
