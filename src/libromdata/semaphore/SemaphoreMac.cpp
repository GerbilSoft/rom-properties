/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SemaphoreMac.cpp: Mac OS X semaphore implementation.                    *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include <mach/mach.h>
#include <mach/mach_traps.h>
#include <mach/semaphore.h>

// C includes (C++ namespace)
#include <cassert>
#include <cerrno>

// References:
// - https://developer.apple.com/library/content/documentation/Darwin/Conceptual/KernelProgramming/synchronization/synchronization.html
// - https://gist.github.com/kazupon/3843288

namespace LibRomData {

class Semaphore
{
public:
	/**
	 * Create a semaphore.
	 * @param count Number of times the semaphore can be obtained before blocking.
	 */
	inline explicit Semaphore(int count);

	/**
	 * Delete the semaphore.
 	 * WARNING: Semaphore MUST be fully released!
	 */
	inline ~Semaphore();

	// Disable copy/assignment constructors.
	Semaphore(const Semaphore &) = delete;
	Semaphore &operator=(const Semaphore &) = delete;

public:
	/**
	 * Obtain the semaphore.
	 * If the semaphore is at zero, this function will block
	 * until another thread releases the semaphore.
	 * @return 0 on success; non-zero on error.
	 */
	inline int obtain(void);

	/**
	 * Release a lock on the semaphore.
	 * @return 0 on success; non-zero on error.
	 */
	inline int release(void);

private:
	semaphore_t m_sem;
};

/**
 * Create a semaphore.
 * @param count Number of times the semaphore can be obtained before blocking.
 */
inline Semaphore::Semaphore(int count)
	: m_sem(0)
{
	kern_return_t ret = semaphore_create(mach_task_self(), &m_sem, SYNC_POLICY_FIFO, count);
	assert(ret == KERN_SUCCESS);
	assert(m_sem != 0);

	// FIXME: Do something if an error occurred here...
	((void)ret);
}

/**
 * Delete the semaphore.
 * WARNING: Semaphore MUST be fully released!
 */
inline Semaphore::~Semaphore()
{
	if (m_sem != 0) {
		semaphore_destroy(mach_task_self(), m_sem);
	}
}

/**
 * Obtain the semaphore.
 * If the semaphore is at zero, this function will block
 * until another thread releases the semaphore.
 * @return 0 on success; non-zero on error.
 */
inline int Semaphore::obtain(void)
{
	if (m_sem != 0)
		return -EBADF;

	return semaphore_wait(m_sem);
}

/**
 * Release a lock on the semaphore.
 * @return 0 on success; non-zero on error.
 */
inline int Semaphore::release(void)
{
	if (m_sem != 0)
		return -EBADF;

	// TODO: What error to return?
	return semaphore_signal(m_sem);
}

} // namespace LibRomData
