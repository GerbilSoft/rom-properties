/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * refbase.hpp: Reference-counted base class.                              *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_REFBASE_HPP__
#define __ROMPROPERTIES_REFBASE_HPP__

// librpthreads
#include "librpthreads/Atomics.h"

// C includes. (C++ namespace)
#include <cassert>

class RefBase
{
	protected:
		RefBase() : m_ref_cnt(1) { }
		virtual ~RefBase() { }

	public:
		/**
		 * Take a reference to this object.
		 * @return this
		 */
		template<class T>
		inline T *ref(void)
		{
			ATOMIC_INC_FETCH(&m_ref_cnt);
			return (T*)this;
		}

		/**
		 * Unreference this object.
		 * If ref_cnt reaches 0, the object is deleted.
		 */
		void unref(void)
		{
			assert(m_ref_cnt > 0);
			if (ATOMIC_DEC_FETCH(&m_ref_cnt) <= 0) {
				// All references removed.
				delete this;
			}
		}

	private:
		volatile int m_ref_cnt;
};

#endif /* __ROMPROPERTIES_REFBASE_HPP__ */
