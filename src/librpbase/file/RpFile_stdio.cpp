/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_stdio.cpp: Standard file object. (stdio implementation)          *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "RpFile.hpp"

// librpbase
#include "byteswap.h"
#include "TextFuncs.hpp"

// C includes. (C++ namespace)
#include <cerrno>

// C++ includes.
#include <string>
using std::string;
using std::u16string;

// zlib for transparent gzip decompression.
#include <zlib.h>
// gzclose_r() and gzclose_w() were introduced in zlib-1.2.4.
#if (ZLIB_VER_MAJOR > 1) || \
    (ZLIB_VER_MAJOR == 1 && ZLIB_VER_MINOR > 2) || \
    (ZLIB_VER_MAJOR == 1 && ZLIB_VER_MINOR == 2 && ZLIB_VER_REVISION >= 4)
// zlib-1.2.4 or later
#else
# define gzclose_r(file) gzclose(file)
# define gzclose_w(file) gzclose(file)
#endif

#ifdef _WIN32
// Windows: _wfopen() requires a Unicode mode string.
typedef wchar_t mode_str_t;
#define _MODE(str) (L ##str)
#include "RpWin32.hpp"
// Needed for using "\\?\" to bypass MAX_PATH.
using std::wstring;
#include "librpbase/ctypex.h"
// _chsize()
#include <io.h>

#else /* !_WIN32 */

// Other: fopen() requires an 8-bit mode string.
typedef char mode_str_t;
#define _MODE(str) (str)
// ftruncate()
#include <unistd.h>
#endif

namespace LibRpBase {

// Deleter for std::unique_ptr<FILE> d->file.
struct myFile_deleter {
	void operator()(FILE *p) const {
		if (p != nullptr) {
			fclose(p);
		}
	}
};

/** RpFilePrivate **/

class RpFilePrivate
{
	public:
		RpFilePrivate(RpFile *q, const char *filename, RpFile::FileMode mode)
			: q_ptr(q), filename(filename), mode(mode), gzfd(nullptr), gzsz(-1) { }
		RpFilePrivate(RpFile *q, const string &filename, RpFile::FileMode mode)
			: q_ptr(q), filename(filename), mode(mode), gzfd(nullptr), gzsz(-1) { }
		~RpFilePrivate();

	private:
		RP_DISABLE_COPY(RpFilePrivate)
		RpFile *const q_ptr;

	public:
		std::shared_ptr<FILE> file;	// File pointer.
		string filename;		// Filename.
		RpFile::FileMode mode;		// File mode.

		gzFile gzfd;			// Used for transparent gzip decompression.
		int64_t gzsz;			// Uncompressed file size.

	public:
		/**
		 * Convert an RpFile::FileMode to an fopen() mode string.
		 * @param mode	[in] FileMode
		 * @return fopen() mode string.
		 */
		static inline const mode_str_t *mode_to_str(RpFile::FileMode mode);

		/**
		 * (Re-)Open the main file.
		 *
		 * INTERNAL FUNCTION. This does NOT affect gzfd.
		 * NOTE: This function sets q->m_lastError.
		 *
		 * Uses parameters stored in this->filename and this->mode.
		 * @return 0 on success; non-zero on error.
		 */
		int reOpenFile(void);
};

RpFilePrivate::~RpFilePrivate()
{
	if (gzfd) {
		gzclose_r(gzfd);
	}
}

/**
 * Convert an RpFile::FileMode to an fopen() mode string.
 * @param mode	[in] FileMode
 * @return fopen() mode string.
 */
inline const mode_str_t *RpFilePrivate::mode_to_str(RpFile::FileMode mode)
{
	switch (mode & RpFile::FM_MODE_MASK) {
		case RpFile::FM_OPEN_READ:
			return _MODE("rb");
		case RpFile::FM_OPEN_WRITE:
			return _MODE("rb+");
		case RpFile::FM_CREATE|RpFile::FM_READ /*RpFile::FM_CREATE_READ*/ :
		case RpFile::FM_CREATE_WRITE:
			return _MODE("wb+");
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
	const mode_str_t *const mode_str = mode_to_str(mode);

#ifdef _WIN32
	// Windows: Use U82W_s() to convert the filename to wchar_t.

	// If this is an absolute path, make sure it starts with
	// "\\?\" in order to support filenames longer than MAX_PATH.
	wstring filenameW;
	if (filename.size() > 3 &&
	    ISASCII(d->filename[0]) && ISALPHA(filename[0]) &&
	    filename[1] == ':' && filename[2] == '\\')
	{
		// Absolute path. Prepend "\\?\" to the path.
		filenameW = L"\\\\?\\";
		filenameW += U82W_s(filename);
	} else {
		// Not an absolute path, or "\\?\" is already
		// prepended. Use it as-is.
		filenameW = U82W_s(filename);
	}

	file.reset(_wfopen(filenameW.c_str(), mode_str), myFile_deleter());
#else /* !_WIN32 */
	// Linux: Use UTF-8 filenames directly.
	file.reset(fopen(filename.c_str(), mode_str), myFile_deleter());
#endif /* _WIN32 */

	// Return 0 if it's *not* nullptr.
	if (!file) {
		RP_Q(RpFile);
		q->m_lastError = errno;
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		return q->m_lastError;
	}
	return 0;
}

/** RpFile **/

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename.
 * @param mode File mode.
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
 * @param filename Filename.
 * @param mode File mode.
 */
RpFile::RpFile(const string &filename, FileMode mode)
	: super()
	, d_ptr(new RpFilePrivate(this, filename, mode))
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

	// Cannot use decompression with writing.
	// FIXME: Proper assert statement...
	//assert((d->mode & FM_MODE_MASK != RpFile::FM_READ) || (d->mode & RpFile::FM_GZIP_DECOMPRESS));

	// Open the file.
	if (d->reOpenFile() != 0) {
		// An error occurred while opening the file.
		return;
	}

	// Check if this is a gzipped file.
	// If it is, use transparent decompression.
	// Reference: https://www.forensicswiki.org/wiki/Gzip
	if (d->mode == FM_OPEN_READ_GZ) {
		uint16_t gzmagic;
		size_t size = fread(&gzmagic, 1, sizeof(gzmagic), d->file.get());
		if (size == sizeof(gzmagic) && gzmagic == be16_to_cpu(0x1F8B)) {
			// This is a gzipped file.
			// Get the uncompressed size at the end of the file.
			fseeko(d->file.get(), 0, SEEK_END);
			int64_t real_sz = ftello(d->file.get());
			if (real_sz > 10+8) {
				int ret = fseeko(d->file.get(), real_sz-4, SEEK_SET);
				if (!ret) {
					uint32_t uncomp_sz;
					size = fread(&uncomp_sz, 1, sizeof(uncomp_sz), d->file.get());
					uncomp_sz = le32_to_cpu(uncomp_sz);
					if (size == sizeof(uncomp_sz) && uncomp_sz >= real_sz-(10+8)) {
						// Uncompressed size looks valid.
						d->gzsz = (int64_t)uncomp_sz;

						// Open the file with gzdopen().
						::rewind(d->file.get());
						::fflush(d->file.get());
						int gzfd_dup = ::dup(fileno(d->file.get()));
						if (gzfd_dup >= 0) {
							d->gzfd = gzdopen(gzfd_dup, "r");
							if (!d->gzfd) {
								// gzdopen() failed.
								// Close the dup()'d handle to prevent a leak.
								::close(gzfd_dup);
							}
						}
					}
				}
			}
		}

		if (!d->gzfd) {
			// Not a gzipped file.
			// Rewind and flush the file.
			::rewind(d->file.get());
			::fflush(d->file.get());
		}
	}
}

RpFile::~RpFile()
{
	delete d_ptr;
}

/**
 * Copy constructor.
 * @param other Other instance.
 */
RpFile::RpFile(const RpFile &other)
	: super()
	, d_ptr(new RpFilePrivate(this, other.d_ptr->filename, other.d_ptr->mode))
{
	RP_D(RpFile);
	m_lastError = other.m_lastError;

	// NOTE: If the file is gzipped, we can't simply dup()
	// the file handle because gzdopen() won't work correctly.
	// TODO: Consolidate with init() and others.
	if (other.d_ptr->gzfd) {
		// Re-open the file.
		if (!d->reOpenFile()) {
			// Open as gzip without checking modes,
			// since we know it was already gzipped.
			::rewind(d->file.get());
			::fflush(d->file.get());
			int gzfd_dup = ::dup(fileno(d->file.get()));
			if (gzfd_dup >= 0) {
				d->gzfd = gzdopen(gzfd_dup, "r");
				if (d->gzfd) {
					// Copy the uncompressed size and seek position.
					d->gzsz = other.d_ptr->gzsz;
					gzseek(d->gzfd, gztell(other.d_ptr->gzfd), SEEK_SET);
				} else {
					// gzdopen() failed.
					// Close the dup()'d handle to prevent a leak.
					::close(gzfd_dup);
				}
			}
		}
	} else {
		// Not gzipped.
		d->file = other.d_ptr->file;
	}
}

/**
 * Assignment operator.
 * @param other Other instance.
 * @return This instance.
 */
RpFile &RpFile::operator=(const RpFile &other)
{
	RP_D(RpFile);
	d->filename = other.d_ptr->filename;
	d->mode = other.d_ptr->mode;
	m_lastError = other.m_lastError;

	// NOTE: If the file is gzipped, we can't simply dup()
	// the file handle because gzdopen() won't work correctly.
	// TODO: Consolidate with init() and others.
	if (other.d_ptr->gzfd) {
		// Re-open the file.
		const mode_str_t *mode_str = d->mode_to_str(d->mode);
#ifdef _WIN32
		// Windows: Use U82W_s() to convert the filename to wchar_t.
		wstring filenameW = U82W_s(d->filename);
		d->file.reset(_wfopen(filenameW.c_str(), mode_str), myFile_deleter());
#else /* !_WIN32 */
		// Linux: Use UTF-8 filenames directly.
		d->file.reset(fopen(d->filename.c_str(), mode_str), myFile_deleter());
#endif /* _WIN32 */
		if (d->file) {
			// Open as gzip without checking modes,
			// since we know it was already gzipped.
			::rewind(d->file.get());
			::fflush(d->file.get());
			int gzfd_dup = ::dup(fileno(d->file.get()));
			if (gzfd_dup >= 0) {
				d->gzfd = gzdopen(gzfd_dup, "r");
				if (d->gzfd) {
					// Copy the uncompressed size and seek position.
					d->gzsz = other.d_ptr->gzsz;
					gzseek(d->gzfd, gztell(other.d_ptr->gzfd), SEEK_SET);
				} else {
					// gzdopen() failed.
					// Close the dup()'d handle to prevent a leak.
					::close(gzfd_dup);
				}
			}
		}
	} else {
		// Not gzipped.
		d->file = other.d_ptr->file;
	}

	return *this;
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpFile::isOpen(void) const
{
	RP_D(const RpFile);
	return (d->file.get() != nullptr);
}

/**
 * dup() the file handle.
 *
 * Needed because IRpFile* objects are typically
 * pointers, not actual instances of the object.
 *
 * NOTE: The dup()'d IRpFile* does NOT have a separate
 * file pointer. This is due to how dup() works.
 *
 * @return dup()'d file, or nullptr on error.
 */
IRpFile *RpFile::dup(void)
{
	return new RpFile(*this);
}

/**
 * Close the file.
 */
void RpFile::close(void)
{
	RP_D(RpFile);
	if (d->gzfd) {
		gzclose_r(d->gzfd);
		d->gzfd = nullptr;
	}
	d->file.reset();
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

	size_t ret;
	if (d->gzfd) {
		int iret = gzread(d->gzfd, ptr, size);
		if (iret >= 0) {
			ret = (size_t)iret;
		} else {
			// An error occurred.
			ret = 0;
			m_lastError = errno;
		}
	} else {
		ret = fread(ptr, 1, size, d->file.get());
		if (ferror(d->file.get())) {
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

	size_t ret = fwrite(ptr, 1, size, d->file.get());
	if (ferror(d->file.get())) {
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
int RpFile::seek(int64_t pos)
{
	RP_D(RpFile);
	if (!d->file) {
		m_lastError = EBADF;
		return -1;
	}

	int ret;
	if (d->gzfd) {
		z_off_t zret = gzseek(d->gzfd, pos, SEEK_SET);
		if (zret >= 0) {
			ret = 0;
		} else {
			// TODO: Does gzseek() set errno?
			ret = -1;
			m_lastError = -EIO;
		}
	} else {
		ret = fseeko(d->file.get(), pos, SEEK_SET);
		if (ret != 0) {
			m_lastError = errno;
		}
	}
	::fflush(d->file.get());	// needed for some things like gzip
	return ret;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
int64_t RpFile::tell(void)
{
	RP_D(RpFile);
	if (!d->file) {
		m_lastError = EBADF;
		return -1;
	}

	if (d->gzfd) {
		return (int64_t)gztell(d->gzfd);
	}
	return ftello(d->file.get());
}

/**
 * Truncate the file.
 * @param size New size. (default is 0)
 * @return 0 on success; -1 on error.
 */
int RpFile::truncate(int64_t size)
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
	int64_t pos = ftello(d->file.get());
	if (pos < 0) {
		m_lastError = errno;
		return -1;
	}

	// Truncate the file.
	fflush(d->file.get());
#ifdef _WIN32
	int ret = _chsize_s(fileno(d->file.get()), size);
#else
	int ret = ftruncate(fileno(d->file.get()), size);
#endif
	if (ret != 0) {
		m_lastError = errno;
		return -1;
	}

	// If the previous position was past the new
	// file size, reset the pointer.
	if (pos > size) {
		ret = fseeko(d->file.get(), size, SEEK_SET);
		if (ret != 0) {
			m_lastError = errno;
			return -1;
		}
	}

	// File truncated.
	return 0;
}

/** File properties. **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
int64_t RpFile::size(void)
{
	RP_D(RpFile);
	if (!d->file) {
		m_lastError = EBADF;
		return -1;
	}

	// TODO: Error checking?

	if (d->gzfd) {
		// gzipped files have the uncompressed size stored
		// at the end of the stream.
		return d->gzsz;
	}

	// Save the current position.
	int64_t cur_pos = ftello(d->file.get());

	// Seek to the end of the file and record its position.
	fseeko(d->file.get(), 0, SEEK_END);
	int64_t end_pos = ftello(d->file.get());

	// Go back to the previous position.
	fseeko(d->file.get(), cur_pos, SEEK_SET);

	// Return the file size.
	return end_pos;
}

/**
 * Get the filename.
 * @return Filename. (May be empty if the filename is not available.)
 */
string RpFile::filename(void) const
{
	RP_D(const RpFile);
	return d->filename;
}

}
