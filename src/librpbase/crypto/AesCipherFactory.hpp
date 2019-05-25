/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AesCipherFactory.hpp: IAesCipher factory class.                         *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_CRYPTO_AESCIPHERFACTORY_HPP__
#define __ROMPROPERTIES_LIBRPBASE_CRYPTO_AESCIPHERFACTORY_HPP__

#include "common.h"

namespace LibRpBase {

class IAesCipher;
class AesCipherFactory
{
	private:
		AesCipherFactory();
		~AesCipherFactory();
	private:
		RP_DISABLE_COPY(AesCipherFactory)

	public:
		/**
		 * Create an IAesCipher class.
		 *
		 * The implementation is chosen depending on the system
		 * environment. The caller doesn't need to know what
		 * the underlying implementation is.
		 *
		 * @return IAesCipher class, or nullptr if decryption isn't supported
		 */
		static IAesCipher *create(void);
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_CRYPTO_AESCIPHERFACTORY_HPP__ */
