/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Semaphore.hpp: System-specific semaphore implementation.                *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// NOTE: The .cpp files are #included here in order to inline the functions.
// Do NOT compile them separately!

// Each .cpp file defines the Mutex class itself, with required fields.

#if defined(_WIN32)
#  include "SemaphoreWin32.cpp"
#elif defined(__APPLE__)
#  include "SemaphoreMac.cpp"
#else
#  include "SemaphorePosix.cpp"
#endif

namespace LibRomData {

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

	// Disable copy/assignment constructors.
	SemaphoreLocker(const SemaphoreLocker &) = delete;
	SemaphoreLocker &operator=(const SemaphoreLocker &) = delete;

private:
	Semaphore &m_sem;
};

} // namespace LibRomData
