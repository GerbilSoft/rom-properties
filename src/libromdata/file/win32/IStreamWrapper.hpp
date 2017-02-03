/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IStreamWrapper.hpp: IStream wrapper for IRpFile. (Win32)                *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_FILE_WIN32_ISTREAMWRAPPER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_FILE_WIN32_ISTREAMWRAPPER_HPP__

#ifndef _WIN32
#error IStreamWrapper.hpp is Windows only.
#endif

#include "../IRpFile.hpp"
#include "../../RpWin32.hpp"
#include <objidl.h>

namespace LibRomData {

class IStreamWrapper : public IStream
{
	public:
		/**
		 * Create an IStream wrapper for IRpFile.
		 * The IRpFile is dup()'d.
		 * @param file IRpFile.
		 */
		explicit IStreamWrapper(IRpFile *file);
		virtual ~IStreamWrapper();

	private:
		typedef IStream super;
		RP_DISABLE_COPY(IStreamWrapper)

	public:
		/**
		 * Get the IRpFile.
		 * NOTE: The IRpFile is still owned by this object.
		 * @return IRpFile.
		 */
		IRpFile *file(void) const;

		/**
		 * Set the IRpFile.
		 * @param file New IRpFile.
		 */
		void setFile(IRpFile *file);

	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) override final;
		IFACEMETHODIMP_(ULONG) AddRef(void) override final;
		IFACEMETHODIMP_(ULONG) Release(void) override final;

		// ISequentialStream
		IFACEMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead) override final;
		IFACEMETHODIMP Write(const void *pv, ULONG cb, ULONG *pcbWritten) override final;

		// IStream
		IFACEMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition) override final;
		IFACEMETHODIMP SetSize(ULARGE_INTEGER libNewSize) override final;
		IFACEMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten) override final;
		IFACEMETHODIMP Commit(DWORD grfCommitFlags) override final;
		IFACEMETHODIMP Revert(void) override final;
		IFACEMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override final;
		IFACEMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override final;
		IFACEMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag) override final;
		IFACEMETHODIMP Clone(IStream **ppstm) override final;

	private:
		/* References of this object. */
		volatile ULONG m_ulRefCount;

	protected:
		IRpFile *m_file;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_FILE_WIN32_ISTREAMWRAPPER_HPP__ */
