/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * Semaphore.hpp: System-specific semaphore implementation.                *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_SEMAPHORE_HPP__
#define __ROMPROPERTIES_LIBRPBASE_SEMAPHORE_HPP__

#include "common.h"

// NOTE: The .cpp files are #included here in order to inline the functions.
// Do NOT compile them separately!

// Each .cpp file defines the Mutex class itself, with required fields.

#ifdef _WIN32
# include "SemaphoreWin32.cpp"
#else /* !_WIN32 */
# include "SemaphorePosix.cpp"
#endif

namespace LibRpBase {

/**
 * Automatic semaphore locker/unlocker class.
 * Obtains the semaphore when created.
 * Releases the semaphore when it goes out of scope.
 */
class SemaphoreLocker
{
	public:
		inline explicit SemaphoreLocker(Semaphore &sem)
			: m_sem(sem)
		{
			m_sem.obtain();
		}

		inline ~SemaphoreLocker()
		{
			m_sem.release();
		}

	private:
		RP_DISABLE_COPY(SemaphoreLocker)

	private:
		Semaphore &m_sem;
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_SEMAPHORE_HPP__ */
