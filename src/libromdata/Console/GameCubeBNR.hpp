/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCubeBNR.hpp: Nintendo GameCube banner reader.                       *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_GAMECUBEBNR_HPP__
#define __ROMPROPERTIES_LIBROMDATA_GAMECUBEBNR_HPP__

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(GameCubeBNR)
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()

	public:
		/** GameCubeBNR accessors. **/

		/**
		 * Add a field for the GameCube banner.
		 *
		 * This adds an RFT_STRING field for BNR1, and
		 * RFT_STRING_MULTI for BNR2.
		 *
		 * @param fields RomFields*
		 * @param gcnRegion GameCube region for BNR1 encoding.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int addField_gameInfo(LibRpBase::RomFields *fields, uint32_t gcnRegion) const;

ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_GAMECUBEBNR_HPP__ */
