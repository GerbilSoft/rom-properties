/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AesCipherFactory.hpp: IAesCipher factory class.                         *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.librpbase.h"
#include "common.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

namespace LibRpBase {

class IAesCipher;
class AesCipherFactory
{
	public:
		// Static class
		AesCipherFactory() = delete;
		~AesCipherFactory() = delete;
	private:
		RP_DISABLE_COPY(AesCipherFactory)

	public:
		/**
		 * Create an IAesCipher object.
		 *
		 * The implementation is chosen depending on the system
		 * environment. The caller doesn't need to know what
		 * the underlying implementation is.
		 *
		 * @return IAesCipher class, or nullptr if decryption isn't supported
		 */
		static IAesCipher *create(void);

	public:
		enum class Implementation {
#ifdef _WIN32
			CAPI,
			CAPI_NG,
#endif /* _WIN32 */
#ifdef HAVE_NETTLE
			Nettle,
#endif /* HAVE_NETTLE */
		};

		/**
		 * Create an IAesCipher object.
		 *
		 * The implementation can be selected by the caller.
		 * This is usually only used for test suites.
		 *
		 * @return IAesCipher class, or nullptr if decryption or the selected implementation isn't supported
		 */
		RP_LIBROMDATA_PUBLIC
		static IAesCipher *create(Implementation implementation);
};

}
