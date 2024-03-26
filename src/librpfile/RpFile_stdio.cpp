/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpFile_stdio.cpp: Standard file object. (stdio implementation)          *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpfile.h"

#ifdef _WIN32
# error RpFile_stdio is not supported on Windows, use RpFile_win32.
#endif /* _WIN32 */

#include "RpFile.hpp"
#include "RpFile_p.hpp"

// librpbyteswap
#include "librpbyteswap/byteswap_rp.h"

// C includes
#include <fcntl.h>	// AT_EMPTY_PATH
#include <sys/stat.h>	// stat(), statx()
#include <unistd.h>	// ftruncate()

namespace LibRpFile {

/** RpFilePrivate **/

RpFilePrivate::RpFilePrivate(RpFile *q, const char *filename, RpFile::FileMode mode)
	: q_ptr(q), file(INVALID_HANDLE_VALUE)
	, mode(mode), gzfd(nullptr), gzsz(-1), devInfo(nullptr)
{
	assert(filename != nullptr);
	this->filename = strdup(filename);
}

RpFilePrivate::~RpFilePrivate()
{
	if (gzfd != nullptr) {
		gzclose_r(gzfd);
	}
	if (file) {
		fclose(file);
	}
	free(filename);
	delete devInfo;
}

/**
 * Convert an RpFile::FileMode to an fopen() mode string.
 * @param mode	[in] FileMode
 * @return fopen() mode string.
 */
inline const char *RpFilePrivate::mode_to_str(RpFile::FileMode mode)
{
	switch (mode & RpFile::FM_MODE_MASK) {
		case RpFile::FM_OPEN_READ:
			return "rb";
		case RpFile::FM_OPEN_WRITE:
			return "rb+";
		case RpFile::FM_CREATE|RpFile::FM_READ /*RpFile::FM_CREATE_READ*/ :
		case RpFile::FM_CREATE_WRITE:
			return "wb+";
		default:
			// Invalid mode.
			return nullptr;
	}
}

/**
 * (Re-)Open the main file.
 *
 * INTERNAL FUNCTION. This does NOT affect gzfd.
 * NOTE: This function sets q->m_lastError.
 *
 * Uses parameters stored in this->filename and this->mode.
 * @return 0 on success; non-zero on error.
 */
int RpFilePrivate::reOpenFile(void)
{
	RP_Q(RpFile);
	const char *const mode_str = mode_to_str(mode);

	// Linux: Use UTF-8 filenames directly.
	if (file) {
		fclose(file);
		file = nullptr;
	}

	// NOTE: Need to call stat() before fopen(), since if the file
	// in question is a pipe, fopen() will hang. (No O_NONBLOCK).
	// This *can* lead to a race condition, but we can't do much
	// about that...

	// Check if this is a device.
	bool hasFileMode = false;
	uint8_t fileType = 0;
#ifdef HAVE_STATX
	struct statx sbx;
	int ret = statx(AT_FDCWD, filename, 0, STATX_TYPE, &sbx);
	if (ret == 0 && (sbx.stx_mask & STATX_TYPE)) {
		// statx() succeeded.
		hasFileMode = true;
		fileType = IFTODT(sbx.stx_mode);
	}
#else /* !HAVE_STATX */
	struct stat sb;
	if (!stat(filename, &sb)) {
		// fstat() succeeded.
		hasFileMode = true;
		fileType = IFTODT(sb.st_mode);
	}
#endif /* HAVE_STATX */

	// Did we get the file mode from statx() or stat()?
	if (hasFileMode) {
		q->m_lastError = 0;
		switch (fileType) {
			case DT_DIR:
				// This is a directory.
				// TODO: How to handle NUS packages?
				q->m_lastError = EISDIR;
				break;

			case DT_REG:
			case DT_BLK:
				// This is a regular file or device file.
				break;

#ifdef __linux__
			case DT_CHR:
				// NOTE: Some Unix systems use character devices for "raw"
				// block devices. Linux does not, so on Linux, we'll only
				// allow block devices and not character devices.
				q->m_lastError = ENOTSUP;
				break;
#endif /* !__linux__ */

			default:
				// Other file types aren't supported.
				q->m_lastError = ENOTSUP;
				break;
		}

		if (q->m_lastError != 0) {
			return -q->m_lastError;
		}
		q->m_fileType = fileType;
	}

	// NOTE: Opening certain device files can cause crashes
	// and/or hangs (e.g. stdin). Only allow device files
	// that match certain patterns.
	// TODO: May need updates for *BSD, Mac OS X, etc.
	// TODO: Check if a block device is a CD-ROM or something else.
	if (q->isDevice()) {
		// Check the filename pattern.
		// TODO: More systems.
		// NOTE: "\x08ExtMedia" is interpreted as a 0x8E byte by both
		// MSVC 2015 and gcc-4.5.2. In order to get it to work correctly,
		// we have to store the length byte separately from the actual
		// image type name.
		static constexpr char fileNamePatterns[][16] = {
#if defined(__linux__)
			"\x07" "/dev/sr",
			"\x08" "/dev/scd",
			"\x0A" "/dev/disk/",
			"\x0B" "/dev/block/",
#elif defined(__FreeBSD__) || defined(__DragonFly__) || \
      defined(__NetBSD__) || defined(__OpenBSD__)
			"\x07" "/dev/cd",
			"\x08" "/dev/rcd",
#else
#  define NO_PATTERNS_FOR_THIS_OS 1
			"\x00"
#endif
		};

#ifndef NO_PATTERNS_FOR_THIS_OS
		bool isMatch = false;
		for (const auto &pattern : fileNamePatterns) {
			if (!strncasecmp(filename, &pattern[1], (uint8_t)pattern[0])) {
				// Found a match!
				isMatch = true;
				break;
			}
		}
		if (!isMatch) {
			// Not a match.
			q->m_lastError = ENOTSUP;
			return -ENOTSUP;
		}
#else
		RP_UNUSED(fileNamePatterns);
#endif /* NO_PATTERNS_FOR_THIS_OS */
	}

	file = fopen(filename, mode_str);

	// If fopen() failed (and returned nullptr),
	// return the non-zero error code.
	if (!file) {
		q->m_lastError = errno;
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		return q->m_lastError;
	}

	if (q->isDevice()) {
		// Allocate devInfo.
		// NOTE: This is kept around until RpFile is deleted,
		// even if the device can't be opeend for some reason.
		devInfo = new DeviceInfo();

		// Get the device size from the OS.
		q->rereadDeviceSizeOS();
	}

	return 0;
}

/** RpFile **/

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename
 * @param mode File mode
 */
RpFile::RpFile(const char *filename, FileMode mode)
	: super()
	, d_ptr(new RpFilePrivate(this, filename, mode))
{
	init();
}

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename
 * @param mode File mode
 */
RpFile::RpFile(const string &filename, FileMode mode)
	: super()
	, d_ptr(new RpFilePrivate(this, filename.c_str(), mode))
{
	init();
}

/**
 * Common initialization function for RpFile's constructors.
 * Filename must be set in d->filename.
 */
void RpFile::init(void)
{
	RP_D(RpFile);

#ifndef NDEBUG
	// Cannot use decompression with writing.
	if (d->mode & RpFile::FM_GZIP_DECOMPRESS) {
		assert((d->mode & FM_MODE_MASK) != RpFile::FM_WRITE);
	}
#endif /* NDEBUG */

	// Open the file.
	if (d->reOpenFile() != 0) {
		// An error occurred while opening the file.
		return;
	}

	m_isWritable = !!(d->mode & RpFile::FM_WRITE);

	// Check if this is a gzipped file.
	// If it is, use transparent decompression.
	// Reference: https://www.forensicswiki.org/wiki/Gzip
	const bool tryGzip = (d->mode == FM_OPEN_READ_GZ);
	if (tryGzip) { do {
		uint16_t gzmagic;
		size_t size = fread(&gzmagic, 1, sizeof(gzmagic), d->file);
		if (size != sizeof(gzmagic) || gzmagic != be16_to_cpu(0x1F8B))
			break;

		// This is a gzipped file.
		// Get the uncompressed size at the end of the file.
		fseeko(d->file, 0, SEEK_END);
		off64_t real_sz = ftello(d->file);
		if (real_sz <= 10+8)
			break;

		if (fseeko(d->file, real_sz-4, SEEK_SET) != 0)
			break;

		uint32_t uncomp_sz;
		size = fread(&uncomp_sz, 1, sizeof(uncomp_sz), d->file);
		uncomp_sz = le32_to_cpu(uncomp_sz);
		if (size != sizeof(uncomp_sz) /*|| uncomp_sz < real_sz-(10+8)*/)
			break;

		// NOTE: Uncompressed size might be smaller than the real filesize
		// in cases where gzip doesn't help much.
		// TODO: Add better verification heuristics?
		d->gzsz = (off64_t)uncomp_sz;

		// Make sure the CRC32 table is initialized.
		get_crc_table();

		// Open the file with gzdopen().
		::rewind(d->file);
		::fflush(d->file);
		int gzfd_dup = ::dup(fileno(d->file));
		if (gzfd_dup >= 0) {
			d->gzfd = gzdopen(gzfd_dup, "r");
			if (d->gzfd) {
				m_isCompressed = true;
			} else {
				// gzdopen() failed.
				// Close the dup()'d handle to prevent a leak.
				::close(gzfd_dup);
			}
		}
	} while (0); }

	if (tryGzip && !d->gzfd) {
		// Not a gzipped file.
		// Rewind and flush the file.
		::rewind(d->file);
		::fflush(d->file);
	}
}

RpFile::~RpFile()
{
	delete d_ptr;
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpFile::isOpen(void) const
{
	RP_D(const RpFile);
	return (d->file != nullptr);
}

/**
 * Close the file.
 */
void RpFile::close(void)
{
	RP_D(RpFile);

	// NOTE: devInfo is not deleted here,
	// since the properties may still be used.
	// We *will* close any handles and free the
	// sector cache, though.
	if (d->devInfo) {
		d->devInfo->close();
	}

	if (d->gzfd != nullptr) {
		gzclose_r(d->gzfd);
		d->gzfd = nullptr;
	}
	if (d->file) {
		fclose(d->file);
		d->file = nullptr;
	}
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpFile::read(void *ptr, size_t size)
{
	RP_D(RpFile);
	if (!d->file) {
		m_lastError = EBADF;
		return 0;
	}

	if (d->devInfo) {
		// Block device. Need to read in multiples of the block size.
		return d->readUsingBlocks(ptr, size);
	}

	size_t ret;
	if (d->gzfd != nullptr) {
		int iret = gzread(d->gzfd, ptr, size);
		if (iret >= 0) {
			ret = (size_t)iret;
		} else {
			// An error occurred.
			ret = 0;
			m_lastError = errno;
		}
	} else {
		ret = fread(ptr, 1, size, d->file);
		if (ferror(d->file)) {
			// An error occurred.
			m_lastError = errno;
		}
	}
	return ret;
}

/**
 * Write data to the file.
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t RpFile::write(const void *ptr, size_t size)
{
	RP_D(RpFile);
	if (!d->file || !(d->mode & FM_WRITE)) {
		// Either the file isn't open,
		// or it's read-only.
		m_lastError = EBADF;
		return 0;
	}

	size_t ret = fwrite(ptr, 1, size, d->file);
	if (ferror(d->file)) {
		// An error occurred.
		m_lastError = errno;
	}
	return ret;
}

/**
 * Set the file position.
 * @param pos File position.
 * @return 0 on success; -1 on error.
 */
int RpFile::seek(off64_t pos)
{
	RP_D(RpFile);
	if (!d->file) {
		m_lastError = EBADF;
		return -1;
	}

	if (d->devInfo) {
		// SetFilePointerEx() *requires* sector alignment when
		// accessing device files. Hence, we'll have to maintain
		// our own device position.
		if (pos < 0) {
			d->devInfo->device_pos = 0;
		} else if (pos <= d->devInfo->device_size) {
			d->devInfo->device_pos = pos;
		} else {
			d->devInfo->device_pos = d->devInfo->device_size;
		}
		return 0;
	}

	int ret;
	if (d->gzfd != nullptr) {
		errno = 0;
		z_off_t zret = gzseek(d->gzfd, pos, SEEK_SET);
		if (zret >= 0) {
			ret = 0;
		} else {
			ret = -1;
			m_lastError = -errno;
			if (m_lastError == 0) {
				m_lastError = -EIO;
			}
		}
	} else {
		ret = fseeko(d->file, pos, SEEK_SET);
		if (ret != 0) {
			m_lastError = errno;
		}
	}
	::fflush(d->file);	// needed for some things like gzip
	return ret;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
off64_t RpFile::tell(void)
{
	RP_D(RpFile);
	if (!d->file) {
		m_lastError = EBADF;
		return -1;
	}

	if (d->gzfd != nullptr) {
		return (off64_t)gztell(d->gzfd);
	}
	return ftello(d->file);
}

/**
 * Truncate the file.
 * @param size New size. (default is 0)
 * @return 0 on success; -1 on error.
 */
int RpFile::truncate(off64_t size)
{
	RP_D(RpFile);
	if (!d->file || !(d->mode & FM_WRITE)) {
		// Either the file isn't open,
		// or it's read-only.
		m_lastError = EBADF;
		return -1;
	} else if (size < 0) {
		m_lastError = EINVAL;
		return -1;
	}

	// Get the current position.
	off64_t pos = ftello(d->file);
	if (pos < 0) {
		m_lastError = errno;
		return -1;
	}

	// Truncate the file.
	::fflush(d->file);
	int ret = ftruncate(fileno(d->file), size);
	if (ret != 0) {
		m_lastError = errno;
		return -1;
	}

	// If the previous position was past the new
	// file size, reset the pointer.
	if (pos > size) {
		ret = fseeko(d->file, size, SEEK_SET);
		if (ret != 0) {
			m_lastError = errno;
			return -1;
		}
	}

	// File truncated.
	return 0;
}

/**
 * Flush buffers.
 * This operation only makes sense on writable files.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpFile::flush(void)
{
	if (isWritable()) {
		RP_D(RpFile);
		int ret = fflush(d->file);
		if (ret != 0) {
			m_lastError = errno;
			return -m_lastError;
		}
		return 0;
	}

	// Ignore flush operations if the file isn't writable.
	return 0;
}

/** File properties **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
off64_t RpFile::size(void)
{
	RP_D(RpFile);
	if (!d->file) {
		m_lastError = EBADF;
		return -1;
	}

	// TODO: Error checking?

	if (d->devInfo) {
		// Block device. Use the cached device size.
		return d->devInfo->device_size;
	} else if (d->gzfd != nullptr) {
		// gzipped files have the uncompressed size stored
		// at the end of the stream.
		return d->gzsz;
	}

	// Save the current position.
	off64_t cur_pos = ftello(d->file);

	// Seek to the end of the file and record its position.
	fseeko(d->file, 0, SEEK_END);
	off64_t end_pos = ftello(d->file);

	// Go back to the previous position.
	fseeko(d->file, cur_pos, SEEK_SET);

	// Return the file size.
	return end_pos;
}

/**
 * Get the filename.
 * @return Filename. (May be nullptr if the filename is not available.)
 */
const char *RpFile::filename(void) const
{
	RP_D(const RpFile);
	return (d->filename != nullptr && d->filename[0] != '\0') ? d->filename : nullptr;
}

/** Extra functions **/

/**
 * Make the file writable.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpFile::makeWritable(void)
{
	if (isCompressed()) {
		// File is compressed. Cannot make it writable.
		return -ENOTSUP;
	} else if (isWritable()) {
		// File is already writable.
		return 0;
	}

	RP_D(RpFile);
	off64_t prev_pos = ftello(d->file);
	fclose(d->file);
	d->file = fopen(d->filename, "rb+");
	if (d->file) {
		// File is now writable.
		m_isWritable = true;
	} else {
		// Failed to open the file as writable.
		// Try reopening as read-only.
		d->file = fopen(d->filename, "rb");
		if (d->file) {
			fseeko(d->file, prev_pos, SEEK_SET);
		}
		return -ENOTSUP;
	}

	// Restore the seek position.
	fseeko(d->file, prev_pos, SEEK_SET);
	d->mode = (RpFile::FileMode)(d->mode | FM_WRITE);
	return 0;
}

}
