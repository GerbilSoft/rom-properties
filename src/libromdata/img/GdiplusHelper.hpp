/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GdiplusHelper.hpp: GDI+ helper class. (Win32)                           *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_IMG_GDIPLUSHELPER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_IMG_GDIPLUSHELPER_HPP__

#ifndef _WIN32
#error GdiplusHelper is Win32 only.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

class GdiplusHelper
{
	private:
		// GdiplusHelper is a static class.
		GdiplusHelper();
		~GdiplusHelper();
		GdiplusHelper(const GdiplusHelper &other);
		GdiplusHelper &operator=(const GdiplusHelper &other);

	public:
		/**
		 * Initialize GDI+.
		 * @return GDI+ token, or 0 on failure.
		 */
		static ULONG_PTR InitGDIPlus(void);

		/**
		 * Shut down GDI+.
		 * @param gdipToken GDI+ token.
		 */
		static void ShutdownGDIPlus(ULONG_PTR gdipToken);
};

/**
 * Class that calls GdiplusHelper::InitGDIPlus() when initialized
 * and GdiplusHelper::ShutdownGDIPlus() when deleted.
 */
class ScopedGdiplus
{
	public:
		ScopedGdiplus()
		{
			m_gdipToken = GdiplusHelper::InitGDIPlus();
		}

		~ScopedGdiplus()
		{
			if (m_gdipToken != 0) {
				GdiplusHelper::ShutdownGDIPlus(m_gdipToken);
			}
		}

		bool isValid(void)
		{
			return (m_gdipToken != 0);
		}

	private:
		ScopedGdiplus(const ScopedGdiplus &other);
		ScopedGdiplus &operator=(const ScopedGdiplus &other);

	protected:
		ULONG_PTR m_gdipToken;
};

#endif /* __ROMPROPERTIES_LIBROMDATA_IMG_GDIPLUSHELPER_HPP__ */
