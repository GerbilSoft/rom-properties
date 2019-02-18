/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox_XBE.hpp: Microsoft Xbox executable reader.                         *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX_XBE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX_XBE_HPP__

#include "librpbase/config.librpbase.h"
#include "librpbase/RomData.hpp"

namespace LibRomData {

class Xbox_XBE_Private;
ROMDATA_DECL_BEGIN(Xbox_XBE)
ROMDATA_DECL_CLOSE()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX_XBE_HPP__ */
