/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCubeBNR.hpp: Nintendo GameCube banner reader.                       *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_GAMECUBEBNR_HPP__
#define __ROMPROPERTIES_LIBROMDATA_GAMECUBEBNR_HPP__

#include "librpbase/RomData.hpp"
#include "gcn_banner.h"

namespace LibRomData {

ROMDATA_DECL_BEGIN(GameCubeBNR)
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()

	public:
		/** GameCubeBNR accessors. **/

		/**
		 * Get the gcn_banner_comment_t.
		 *
		 * For BNR2, this returns the comment that most closely
		 * matches the system language.
		 *
		 * return gcn_banner_comment_t, or nullptr on error.
		 */
		const gcn_banner_comment_t *getComment(void) const;

ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_GAMECUBEBNR_HPP__ */
