/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * IStreamWrapper.hpp: IStream wrapper for IRpFile. (Win32)                *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifndef _WIN32
#  error IStreamWrapper.hpp is Windows only.
#endif

#include "../IRpFile.hpp"
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/ComBase.hpp"
#include <objidl.h>

namespace LibRpFile {

class IStreamWrapper final : public LibWin32Common::ComBase<IStream>
{
	public:
		/**
		 * Create an IStream wrapper for IRpFile.
		 * The IRpFile is dup()'d.
		 * @param file IRpFile.
		 */
		explicit IStreamWrapper(const LibRpFile::IRpFilePtr &file)
			: m_file(file)
		{}
	public:
		~IStreamWrapper() final = default;

	private:
		typedef LibWin32Common::ComBase<IStream> super;
		RP_DISABLE_COPY(IStreamWrapper)

	public:
		/**
		 * Get the IRpFile.
		 * NOTE: The IRpFile is still owned by this object.
		 * @return IRpFile.
		 */
		inline IRpFile *file(void) const
		{
			return m_file.get();
		}

		/**
		 * Set the IRpFile.
		 * @param file New IRpFile.
		 */
		void setFile(const LibRpFile::IRpFilePtr &file)
		{
			m_file = file;
		}

	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) final;

		// ISequentialStream
		IFACEMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead) final;
		IFACEMETHODIMP Write(const void *pv, ULONG cb, ULONG *pcbWritten) final;

		// IStream
		IFACEMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition) final;
		IFACEMETHODIMP SetSize(ULARGE_INTEGER libNewSize) final;
		IFACEMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten) final;
		IFACEMETHODIMP Commit(DWORD grfCommitFlags) final;
		IFACEMETHODIMP Revert(void) final;
		IFACEMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) final;
		IFACEMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) final;
		IFACEMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag) final;
		IFACEMETHODIMP Clone(IStream **ppstm) final;

	protected:
		LibRpFile::IRpFilePtr m_file;
};

}
