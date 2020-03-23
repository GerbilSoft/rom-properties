/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpFile_windres.hpp: Windows resource wrapper for IRpFile. (Win32)       *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPFILE_WIN32_RPFILE_WINDRES_HPP__
#define __ROMPROPERTIES_LIBRPFILE_WIN32_RPFILE_WINDRES_HPP__

#ifndef _WIN32
#error RpFile_Resource.hpp is Windows only.
#endif

#include "../RpMemFile.hpp"
#include "libwin32common/RpWin32_sdk.h"

namespace LibRpFile {

class RpFile_windres : public RpMemFile
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
		typedef RpMemFile super;
		RP_DISABLE_COPY(RpFile_windres)

	public:
		/**
		 * Close the file.
		 */
		void close(void) final;

	protected:
		HGLOBAL m_hGlobal;	// Resource handle.
};

}

#endif /* __ROMPROPERTIES_LIBRPFILE_WIN32_RPFILE_WINDRES_HPP__ */
