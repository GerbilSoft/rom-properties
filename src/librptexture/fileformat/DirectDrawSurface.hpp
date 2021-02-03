/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * DirectDrawSurface.hpp: DirectDraw Surface image reader.                 *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_DIRECTDRAWSURFACE_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_DIRECTDRAWSURFACE_HPP__

#include "FileFormat.hpp"

namespace LibRpTexture {

FILEFORMAT_DECL_BEGIN(DirectDrawSurface)

	public:
		static int isRomSupported_static(const DetectInfo *info);

FILEFORMAT_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_DIRECTDRAWSURFACE_HPP__ */
