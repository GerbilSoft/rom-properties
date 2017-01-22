/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AesCipherFactory.hpp: IAesCipher factory class.                         *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_CRYPTO_AESCIPHERFACTORY_HPP__
#define __ROMPROPERTIES_LIBROMDATA_CRYPTO_AESCIPHERFACTORY_HPP__

#include "common.h"

namespace LibRomData {

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
		static IAesCipher *getInstance(void);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_CRYPTO_AESCIPHERFACTORY_HPP__ */
