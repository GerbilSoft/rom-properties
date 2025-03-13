/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * IRpFile.hpp: File wrapper interface.                                    *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes
#include <sys/types.h>	// for off64_t

// C includes (C++ namespace)
#include <cerrno>
#include <cstddef>	// for size_t
#include <cstdint>

// C++ includes
#include <memory>
#include <string>

// Common macros
#include "common.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC
#include "d_type.h"

namespace LibRpFile {

#ifdef _WIN32
template<typename CharType>
static inline constexpr bool T_IsDriveLetter(CharType letter)
{
	return (letter >= (CharType)'A') && (letter <= (CharType)'Z');
}

static inline constexpr bool IsDriveLetterA(char letter)
{
	return T_IsDriveLetter(letter);
}
static inline constexpr bool IsDriveLetterW(wchar_t letter)
{
	return T_IsDriveLetter(letter);
}
#endif /* _WIN32 */

class RP_LIBROMDATA_PUBLIC NOVTABLE IRpFile
{
protected:
	explicit IRpFile();
public:
	virtual ~IRpFile() = default;

private:
	RP_DISABLE_COPY(IRpFile)

public:
	/**
	 * Is the file open?
	 * This usually only returns false if an error occurred.
	 * @return True if the file is open; false if it isn't.
	 */
	virtual bool isOpen(void) const = 0;

	/**
	 * Get the last error.
	 * @return Last POSIX error, or 0 if no error.
	 */
	inline int lastError(void) const
	{
		return m_lastError;
	}

	/**
	 * Clear the last error.
	 */
	inline void clearError(void)
	{
		m_lastError = 0;
	}

	/**
	 * Is the file compressed?
	 * @return True if writable; false if not.
	 */
	inline bool isWritable(void) const
	{
		return m_isWritable;
	}

	/**
	 * Is the file compressed?
	 * If it is, then we're using a transparent decompression
	 * wrapper, so it can't be written to easily.
	 * @return True if compressed; false if not.
	 */
	inline bool isCompressed(void) const
	{
		return m_isCompressed;
	}

	/**
	 * Get the file type.
	 * File types must be set by the IRpFile subclass.
	 * @return DT_* file enumeration, or 0 if unknown.
	 */
	inline uint8_t fileType(void) const
	{
		return m_fileType;
	}

	/**
	 * Is this file a device?
	 * @return True if this is a device; false if not.
	 */
	inline bool isDevice(void) const
	{
		return (m_fileType == DT_BLK || m_fileType == DT_CHR);
	}

public:
	/**
	 * Close the file.
	 */
	virtual void close(void) = 0;

	/**
	 * Read data from the file.
	 * @param ptr	[out] Output data buffer.
	 * @param size	[in] Amount of data to read, in bytes.
	 * @return Number of bytes read.
	 */
	ATTR_ACCESS_SIZE(write_only, 2, 3)
	virtual size_t read(void *ptr, size_t size) = 0;

	/**
	 * Write data to the file.
	 * @param ptr	[in] Input data buffer.
	 * @param size	[in] Amount of data to write, in bytes.
	 * @return Number of bytes written.
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	virtual size_t write(const void *ptr, size_t size) = 0;

	/**
	 * Set the file position.
	 * @param pos	[in] File position.
	 * @return 0 on success; -1 on error.
	 */
	virtual int seek(off64_t pos) = 0;

	/**
	 * Seek to the beginning of the file.
	 */
	inline void rewind(void)
	{
		this->seek(0);
	}

	/**
	 * Get the file position.
	 * @return File position, or -1 on error.
	 */
	virtual off64_t tell(void) = 0;

	/**
	 * Truncate the file.
	 * @param size New size. (default is 0)
	 * @return 0 on success; -1 on error.
	 */
	virtual int truncate(off64_t size = 0)
	{
		// Not supported.
		RP_UNUSED(size);
		m_lastError = ENOTSUP;
		return -1;
	}

	/**
	 * Flush buffers.
	 * This operation only makes sense on writable files.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	virtual int flush(void)
	{
		return -ENOTSUP;
	}

public:
	/** File properties **/

	/**
	 * Get the file size.
	 * @return File size, or negative on error.
	 */
	virtual off64_t size(void) = 0;

	/**
	 * Get the filename.
	 * @return Filename. (May be nullptr if the filename is not available.)
	 */
	virtual const char *filename(void) const
	{
		return nullptr;
	}

public:
	/** Extra functions **/

	/**
	 * Make the file writable.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	virtual int makeWritable(void)
	{
		return -ENOTSUP;
	}

public:
	/** Convenience functions implemented for all IRpFile subclasses **/

	/**
	 * Get a single character (byte) from the file
	 * @return Character from file, or EOF on end of file or error.
	 */
	int getc(void);

	/**
	 * Un-get a single character (byte) from the file.
	 *
	 * Note that this implementation doesn't actually
	 * use a character buffer; it merely decrements the
	 * seek pointer by 1.
	 *
	 * @param c Character. (ignored!)
	 * @return 0 on success; non-zero on error.
	 */
	int ungetc(int c);

	/**
	 * Seek to the specified address, then read data.
	 * @param pos	[in] Requested seek address.
	 * @param ptr	[out] Output data buffer.
	 * @param size	[in] Amount of data to read, in bytes.
	 * @return Number of bytes read on success; 0 on seek or read error.
	 */
	ATTR_ACCESS_SIZE(write_only, 3, 4)
	inline size_t seekAndRead(off64_t pos, void *ptr, size_t size)
	{
		const int ret = this->seek(pos);
		if (ret != 0) {
			// Seek error.
			return 0;
		}
		return this->read(ptr, size);
	}

	/**
	 * Seek to the specified address, then write data.
	 * @param pos	[in] Requested seek address.
	 * @param ptr	[in] Input data buffer.
	 * @param size	[in] Amount of data to write, in bytes.
	 * @return Number of bytes read on success; 0 on seek or read error.
	 */
	ATTR_ACCESS_SIZE(read_only, 3, 4)
	inline size_t seekAndWrite(off64_t pos, const void *ptr, size_t size)
	{
		const int ret = this->seek(pos);
		if (ret != 0) {
			// Seek error.
			return 0;
		}
		return this->write(ptr, size);
	}

	/**
	 * Seek to a relative offset. (SEEK_CUR)
	 * @param pos Relative offset
	 * @return 0 on success; -1 on error
	 */
	inline int seek_cur(off64_t offset)
	{
		const off64_t pos = this->tell();
		if (pos < 0) {
			return -1;
		}
		return this->seek(pos + offset);
	}

	/**
	 * Copy data from this IRpFile to another IRpFile.
	 * Read/write positions must be set before calling this function.
	 * @param pDestFile	[in] Destination IRpFile.
	 * @param size		[in] Number of bytes to copy.
	 * @param pcbRead	[out,opt] Number of bytes read.
	 * @param pcbWritten	[out,opt] Number of bytes written.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int copyTo(IRpFile *pDestFile, off64_t size,
		off64_t *pcbRead = nullptr, off64_t *pcbWritten = nullptr);

protected:
	int m_lastError;	// Last error number (errno)
	bool m_isWritable;	// Is this file writable?
	bool m_isCompressed;	// Is this file compressed?
	uint8_t m_fileType;	// File type (see d_type.h)

private:
	uint8_t m_padding;	// pad to 8 bytes
};

typedef std::shared_ptr<IRpFile> IRpFilePtr;

}
