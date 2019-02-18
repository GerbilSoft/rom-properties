/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_stdio_p.hpp: Standard file object. (PRIVATE CLASS)               *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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

// C includes. (C++ namespace)
#include <cerrno>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

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
// Windows: _tfopen() requires a TCHAR mode string.
typedef TCHAR mode_str_t;
#define _MODE(str) _T(##str)
#include "RpWin32.hpp"
// Needed for using "\\\\?\\" to bypass MAX_PATH.
using std::string;
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

/** RpFilePrivate **/

class RpFilePrivate
{
	public:
		RpFilePrivate(RpFile *q, const char *filename, RpFile::FileMode mode)
			: q_ptr(q), file(nullptr), filename(filename)
			, mode(mode), isDevice(false)
			, gzfd(nullptr), gzsz(-1) { }
		RpFilePrivate(RpFile *q, const string &filename, RpFile::FileMode mode)
			: q_ptr(q), file(nullptr), filename(filename)
			, mode(mode), isDevice(false)
			, gzfd(nullptr), gzsz(-1) { }
		~RpFilePrivate();

	private:
		RP_DISABLE_COPY(RpFilePrivate)
		RpFile *const q_ptr;

	public:
		FILE *file;		// File pointer.
		string filename;	// Filename.
		RpFile::FileMode mode;	// File mode.
		bool isDevice;		// Is this a device file?

		gzFile gzfd;		// Used for transparent gzip decompression.
		int64_t gzsz;		// Uncompressed file size.

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

}
