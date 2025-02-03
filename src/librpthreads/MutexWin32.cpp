/***************************************************************************
 * ROM Properties Page shell extension. (librpthreads)                     *
 * MutexWin32.cpp: Win32 mutex implementation.                             *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>

// SAL 2.0 annotations not supported by Windows SDK 7.1A. (MSVC 2010)
#ifndef _Acquires_lock_
#  define _Acquires_lock_(lock)
#endif
#ifndef _Releases_lock_
#  define _Releases_lock_(lock)
#endif

namespace LibRpThreads {

class Mutex
{
public:
	/**
	 * Create a mutex.
	 */
	inline explicit Mutex();

	/**
	 * Delete the mutex.
	 * WARNING: Mutex MUST be unlocked!
	 */
	inline ~Mutex();

	// Disable copy/assignment constructors.
	Mutex(const Mutex &) = delete;
	Mutex &operator=(const Mutex &) = delete;

public:
	/**
	 * Lock the mutex.
	 * If the mutex is locked, this function will block until
	 * the previous locker unlocks it.
	 * @return 0 on success; non-zero on error.
	 */
	_Acquires_lock_(this->m_criticalSection) inline int lock(void);

	/**
	 * Unlock the mutex.
	 * @return 0 on success; non-zero on error.
	 */
	_Releases_lock_(this->m_criticalSection) inline int unlock(void);

private:
	// NOTE: Windows implementation uses critical sections,
	// since they have less overhead than mutexes.
	CRITICAL_SECTION m_criticalSection;
};

/**
 * Create a mutex.
 */
inline Mutex::Mutex()
{
	// Reference: https://docs.microsoft.com/en-us/windows/win32/sync/using-critical-section-objects
	InitializeCriticalSectionAndSpinCount(&m_criticalSection, 0x400);
}

/**
 * Delete the mutex.
 * WARNING: Mutex MUST be unlocked!
 */
inline Mutex::~Mutex()
{
	// TODO: Error checking.
	DeleteCriticalSection(&m_criticalSection);
}

/**
 * Lock the mutex.
 * If the mutex is locked, this function will block until
 * the previous locker unlocks it.
 * @return 0 on success; non-zero on error.
 */
_Acquires_lock_(this->m_criticalSection) inline int Mutex::lock(void)
{
	// TODO: Error handling?
	EnterCriticalSection(&m_criticalSection);
	return 0;
}

/**
 * Unlock the mutex.
 * @return 0 on success; non-zero on error.
 */
_Releases_lock_(this->m_criticalSection) inline int Mutex::unlock(void)
{
	// TODO: Error handling?
	LeaveCriticalSection(&m_criticalSection);
	return 0;
}

}
