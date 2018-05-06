/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * Mutex.hpp: System-specific mutex implementation.                        *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_MUTEX_HPP__
#define __ROMPROPERTIES_LIBRPBASE_MUTEX_HPP__

#include "common.h"

// NOTE: The .cpp files are #included here in order to inline the functions.
// Do NOT compile them separately!

// Each .cpp file defines the Mutex class itself, with required fields.

#ifdef _WIN32
# include "MutexWin32.cpp"
#else /* !_WIN32 */
# include "MutexPosix.cpp"
#endif

namespace LibRpBase {

/**
 * Automatic mutex locker/unlocker class.
 * Locks the mutex when created.
 * Unlocks the mutex when it goes out of scope.
 */
class MutexLocker
{
	public:
		inline explicit MutexLocker(Mutex &mutex)
			: m_mutex(mutex)
		{
			m_mutex.lock();
		}

		inline ~MutexLocker()
		{
			m_mutex.unlock();
		}

	private:
		RP_DISABLE_COPY(MutexLocker)

	private:
		Mutex &m_mutex;
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_MUTEX_HPP__ */
