/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * DX10Formats.hpp: DirectX 10 formats.                                    *
 *                                                                         *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_DATA_DX10FORMATS_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_DATA_DX10FORMATS_HPP__

#include "librpbase/common.h"

namespace LibRpTexture {

class DX10Formats
{
	private:
		// Static class.
		DX10Formats();
		~DX10Formats();
		RP_DISABLE_COPY(DX10Formats)

	public:
		/**
		 * Look up a DirectX 10 format value.
		 * @param dxgiFormat	[in] DXGI_FORMAT
		 * @return String, or nullptr if not found.
		 */
		static const char *lookup_dxgiFormat(unsigned int dxgiFormat);
};

}

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_DATA_DX10FORMATS_HPP__ */
