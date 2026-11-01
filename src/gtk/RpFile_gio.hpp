/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RpFile_gio.hpp: IRpFile implementation using GIO/GVfs.                  *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// librpfile
#include "librpfile/IRpFile.hpp"

// C++ STL classes
#include "rp-string_view.hpp"

class RpFileGioPrivate;
class RpFileGio final : public LibRpFile::IRpFile
{
public:
	enum FileMode : uint8_t {
		// copied from RpFile
		FM_READ = 0,		// Read-only.
		FM_WRITE = 1,		// Read/write.
		FM_OPEN = 0,		// Open the file. (Must exist!)
		FM_CREATE = 2,		// Create the file. (Will overwrite!)

		// Combinations
		FM_OPEN_READ = 0,	// Open for reading. (Must exist!)
		FM_OPEN_WRITE = 1,	// Open for reading/writing. (Must exist!)
		//FM_CREATE_READ = 2,	// Not valid; handled as FM_CREATE_WRITE.
		FM_CREATE_WRITE = 3,	// Create for reading/writing. (Will overwrite!)

		// Mask
		FM_MODE_MASK = 3,	// Mode mask.

		// TODO: Implement transparent decompression?
#if 0
		// Extras.
		FM_GZIP_DECOMPRESS = 4,	// Transparent gzip decompression. (read-only!)
		FM_OPEN_READ_GZ = FM_READ | FM_GZIP_DECOMPRESS,
#endif
	};

	/**
	 * Open a file.
	 * NOTE: Files are always opened as read-only in binary mode.
	 * @param uri GVfs URI
	 * @param mode File mode
	 */
	explicit RpFileGio(std::string_view uri, FileMode mode);

public:
	~RpFileGio() final;

private:
	typedef LibRpFile::IRpFile super;
protected:
	friend class RpFileGioPrivate;
	RpFileGioPrivate *const d_ptr;
public:
	RP_DISABLE_COPY(RpFileGio)

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
	 * (NOTE: Not valid for RpFileGio; this will always return 0.)
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

public:
	/** File properties **/

	/**
	 * Get the file size.
	 * @return File size, or negative on error.
	 */
	off64_t size(void) final;

	/**
	 * Get the filename.
	 * NOTE: For RpFileGio, this returns a GVfs URI.
	 * @return Filename. (May be nullptr if the filename is not available.)
	 */
	const char *filename(void) const final;
};
