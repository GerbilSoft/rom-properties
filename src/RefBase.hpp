/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * refbase.hpp: Reference-counted base class.                              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "dll-macros.h"

// librpthreads
#include "librpthreads/Atomics.h"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <type_traits>

/**
 * Unreference a RefBase object if it's not NULL.
 */
#define UNREF(obj) \
	do { \
		if (obj) { \
			(obj)->unref(); \
		} \
	} while (0)

/**
 * Unreference a RefBase object if it's not NULL,
 * then set the variable to NULL.
 */
#define UNREF_AND_NULL(obj) \
	do { \
		if (obj) { \
			(obj)->unref(); \
			(obj) = nullptr; \
		} \
	} while (0)

/**
 * Unreference a RefBase object if it's not NULL,
 * then set the variable to NULL.
 * No NULL check is performed.
 */
#define UNREF_AND_NULL_NOCHK(obj) \
	do { \
		(obj)->unref(); \
		(obj) = nullptr; \
	} while (0)

class RP_LIBROMDATA_PUBLIC RefBase
{
	protected:
		RefBase() : m_ref_cnt(1) { }
		virtual ~RefBase() = default;

	public:
		/**
		 * Take a reference to this object.
		 * @return this
		 */
		template<class T>
		inline T *ref(void)
		{
			ATOMIC_INC_FETCH(&m_ref_cnt);
			return static_cast<T*>(this);
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

/**
 * unique_ptr<>-style class for RefBase subclasses.
 * Takes the implied ref() for the RefBase subclass,
 * and unref()'s it when it goes out of scope.
 */
template<class T>
class unique_RefBase
{
	public:
		inline explicit unique_RefBase(T *refBase)
			: m_refBase(refBase)
		{
			static_assert(std::is_base_of<RefBase, T>::value,
				"unique_RefBase<> only supports RefBase subclasses");
		}

		inline ~unique_RefBase()
		{
			UNREF(m_refBase);
		}

		/**
		 * Get the RefBase*.
		 * @return RefBase*
		 */
		inline T *get(void) { return m_refBase; }

		/**
		 * Release the IRpFile*.
		 * This class will reset its pointer to nullptr,
		 * and won't unref() the IRpFile* on destruction.
		 * @return IRpFile*
		 */
		inline T *release(void)
		{
			T *const ptr = m_refBase;
			m_refBase = nullptr;
			return ptr;
		}

		/**
		 * Dereference the RefBase*.
		 * @return RefBase*
		 */
		inline T *operator->(void) const { return m_refBase; }

		/**
		 * Is the RefBase* valid or nullptr?
		 * @return True if valid; false if nullptr.
		 */
		operator bool(void) const { return (m_refBase != nullptr); }

		// Disable copy/assignment constructors.
#if __cplusplus >= 201103L
	public:
		unique_RefBase(const unique_RefBase &) = delete;
		unique_RefBase &operator=(const unique_RefBase &) = delete;
#else
	private:
		unique_RefBase(const unique_RefBase &);
		unique_RefBase &operator=(const unique_RefBase &);
#endif

	private:
		T *m_refBase;
};
