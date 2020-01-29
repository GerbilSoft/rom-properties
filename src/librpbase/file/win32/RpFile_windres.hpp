/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_windres.hpp: Windows resource wrapper for IRpFile. (Win32)       *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_FILE_WIN32_RPFILE_WINDRES_HPP__
#define __ROMPROPERTIES_LIBRPBASE_FILE_WIN32_RPFILE_WINDRES_HPP__

#ifndef _WIN32
#error RpFile_Resource.hpp is Windows only.
#endif

#include "../IRpFile.hpp"
#include "libwin32common/RpWin32_sdk.h"

namespace LibRpBase {

class RpFile_windres : public IRpFile
{
	public:
		/**
		 * Open an IRpFile backed by a Win32 resource.
		 * The resulting IRpFile is read-only.
		 *
		 * @param hModule Module handle.
		 * @param lpName Resource name.
		 * @param lpType Resource type.
		 */
		RpFile_windres(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType);
	protected:
		virtual ~RpFile_windres();	// call unref() instead

	private:
		typedef IRpFile super;
		RP_DISABLE_COPY(RpFile_windres)

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
		size_t read(void *ptr, size_t size) final;

		/**
		 * Write data to the file.
		 * (NOTE: Not valid for RpFile_windres; this will always return 0.)
		 * @param ptr Input data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes written.
		 */
		size_t write(const void *ptr, size_t size) final;

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(int64_t pos) final;

		/**
		 * Get the file position.
		 * @return File position, or -1 on error.
		 */
		int64_t tell(void) final;

		/**
		 * Truncate the file.
		 * (NOTE: Not valid for RpFile_windres; this will always return -1.)
		 * @param size New size. (default is 0)
		 * @return 0 on success; -1 on error.
		 */
		int truncate(int64_t size = 0) final;

	public:
		/** File properties **/

		/**
		 * Get the file size.
		 * @return File size, or negative on error.
		 */
		int64_t size(void) final;

		/**
		 * Get the filename.
		 * @return Filename. (May be empty if the filename is not available.)
		 */
		std::string filename(void) const final;

	protected:
		HGLOBAL m_hGlobal;	// Resource handle.
		const void *m_lpData;	// Resource pointer.
		DWORD m_dwSize;		// Resource size.
		DWORD m_dwPos;		// Current position.
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_FILE_WIN32_RPFILE_WINDRES_HPP__ */
