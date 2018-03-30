/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DSSysTitles.cpp: Nintendo 3DS system title lookup.             *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#include "Nintendo3DSSysTitles.hpp"
#include "libi18n/i18n.h"

// C includes.
#include <stdlib.h>

namespace LibRomData {

class Nintendo3DSSysTitlesPrivate
{
	private:
		// Static class.
		Nintendo3DSSysTitlesPrivate();
		~Nintendo3DSSysTitlesPrivate();
		RP_DISABLE_COPY(Nintendo3DSSysTitlesPrivate)

	public:
		// System title groups are split up by tid_hi.
		// New3DS-specific is indicated by $x0000000, where x == 2.
		struct SysTitle {
			uint32_t tid_lo[6];	// 6 regions. (If 0, not available.)
			const char *desc;	// Description.
		};

		/* TODO: Use this if more groups are added.
		struct SysTitleGroup {
			uint32_t tid_hi;
			const SysTitle *titles;
			int titles_size;
		}; */

		// Regions.
		static const char *const regions[6];

		static const SysTitle sys_title_00040010[];	// System applications.
		static const SysTitle sys_title_00040030[];	// System applets.

		//static const SysTitleGroup sys_title_group[];	// All SysTitle[] arrays.
};

/** Nintendo3DSSysTitlesPrivate **/

// Regions.
// TODO: char array instead of pointers.
const char *const Nintendo3DSSysTitlesPrivate::regions[6] = {
	"JPN", "USA", "EUR",
	"CHN", "KOR", "TWN",
};

/**
 * System applications. (tid hi == 0x00040010)
 */
const Nintendo3DSSysTitlesPrivate::SysTitle Nintendo3DSSysTitlesPrivate::sys_title_00040010[] = {
	/** Common titles **/
	{{0x00020000, 0x00021000, 0x00022000, 0x00026000, 0x00027000, 0x00028000}, NOP_C_("Nintendo3DSSysTitles", "System Settings")},
	{{0x00020100, 0x00021100, 0x00022100, 0x00026100, 0x00027100, 0x00028100}, NOP_C_("Nintendo3DSSysTitles", "Download Play")},
	{{0x00020200, 0x00021200, 0x00022200, 0x00026200, 0x00027200, 0x00028200}, NOP_C_("Nintendo3DSSysTitles", "Activity Log")},
	{{0x00020300, 0x00021300, 0x00022300, 0x00026300, 0x00027300, 0x00028300}, NOP_C_("Nintendo3DSSysTitles", "Health and Safety Information")},
	{{0x00020400, 0x00021400, 0x00022400, 0x00026400, 0x00027400, 0x00028400}, NOP_C_("Nintendo3DSSysTitles", "Nintendo 3DS Camera")},
	{{0x00020500, 0x00021500, 0x00022500, 0x00026500, 0x00027500, 0x00028500}, NOP_C_("Nintendo3DSSysTitles", "Nintendo 3DS Sound")},
	{{0x00020700, 0x00021700, 0x00022700, 0x00026700, 0x00027700, 0x00028700}, NOP_C_("Nintendo3DSSysTitles", "Mii Maker")},
	{{0x00020800, 0x00021800, 0x00022800, 0x00026800, 0x00027800, 0x00028800}, NOP_C_("Nintendo3DSSysTitles", "StreetPass Mii Plaza")},
	{{0x00020900, 0x00021900, 0x00022900,          0, 0x00027900, 0x00028900}, NOP_C_("Nintendo3DSSysTitles", "eShop")},
	{{0x00020A00, 0x00021A00, 0x00022A00,          0, 0x00027A00, 0x00028A00}, NOP_C_("Nintendo3DSSysTitles", "System Transfer")},
	{{0x00020B00, 0x00021B00, 0x00022B00,          0,          0,          0}, NOP_C_("Nintendo3DSSysTitles", "Nintendo Zone")},
	{{0x00020D00, 0x00021D00, 0x00022D00, 0x00026D00, 0x00027D00, 0x00028D00}, NOP_C_("Nintendo3DSSysTitles", "Face Raiders")},
	{{0x00020E00, 0x00021E00, 0x00022E00, 0x00026E00, 0x00027E00, 0x00028E00}, NOP_C_("Nintendo3DSSysTitles", "AR Games")},
	{{0x00020F00, 0x00021F00, 0x00022F00, 0x00026F00, 0x00027F00, 0x00028F00}, NOP_C_("Nintendo3DSSysTitles", "System Updater (SAFE_MODE)")},
	{{0x00023000, 0x00024000, 0x00025000,          0,          0,          0}, NOP_C_("Nintendo3DSSysTitles", "Promotional Video (v1.1.0)")},
	{{0x0002BF00, 0x0002C000, 0x0002C100,          0,          0,          0}, NOP_C_("Nintendo3DSSysTitles", "Nintendo Network ID Settings")},

	/** New 3DS exclusive **/
	{{0x20020300, 0x20021300, 0x20022300,          0, 0x20027300,          0}, NOP_C_("Nintendo3DSSysTitles", "Health and Safety Information")},
	{{0x20020D00, 0x20021D00, 0x20022D00,          0, 0x20027D00,          0}, NOP_C_("Nintendo3DSSysTitles", "Face Raiders")},
	{{0x20023100, 0x20024100, 0x20025100,          0,          0,          0}, NOP_C_("Nintendo3DSSysTitles", "microSD Management")},
};

/**
 * System applets. (tid hi == 0x00040030)
 */
const Nintendo3DSSysTitlesPrivate::SysTitle Nintendo3DSSysTitlesPrivate::sys_title_00040030[] = {
	/** Common titles **/
	{{0x00008202, 0x00008F02, 0x00009802, 0x0000A102, 0x0000A902, 0x0000B102}, NOP_C_("Nintendo3DSSysTitles", "HOME Menu")},
	{{0x00008402, 0x00009002, 0x00009902, 0x0000A202, 0x0000AA02, 0x0000B202}, NOP_C_("Nintendo3DSSysTitles", "Camera")},
	{{0x00008602, 0x00009202, 0x00009B02, 0x0000A402, 0x0000AC02, 0x0000B402}, NOP_C_("Nintendo3DSSysTitles", "Instruction Manual")},
	{{0x00008702, 0x00009302, 0x00009C02, 0x0000A502, 0x0000AD02, 0x0000B502}, NOP_C_("Nintendo3DSSysTitles", "Game Notes")},
	{{0x00008802, 0x00009402, 0x00009D02, 0x0000A602, 0x0000AE02, 0x0000B602}, NOP_C_("Nintendo3DSSysTitles", "Internet Browser")},
	{{0x00008D02, 0x00009602, 0x00009F02, 0x0000A702, 0x0000AF02, 0x0000B702}, NOP_C_("Nintendo3DSSysTitles", "Friend List")},
	{{0x00008E02, 0x00009702, 0x0000A002, 0x0000A802, 0x0000B002, 0x0000B802}, NOP_C_("Nintendo3DSSysTitles", "Notificaitons")},
	{{0x0000C002, 0x0000C802, 0x0000D002, 0x0000D802, 0x0000DE02, 0x0000E402}, NOP_C_("Nintendo3DSSysTitles", "Software Keyboard")},
	{{0x0000C003, 0x0000C803, 0x0000D003, 0x0000D803, 0x0000DE03, 0x0000E403}, NOP_C_("Nintendo3DSSysTitles", "Software Keyboard (SAFE_MODE)")},
	{{0x0000C102, 0x0000C902, 0x0000D102, 0x0000D902, 0x0000DF02, 0x0000E502}, NOP_C_("Nintendo3DSSysTitles", "Mii picker")},
	{{0x0000C302, 0x0000CB02, 0x0000D302, 0x0000DB02, 0x0000E102, 0x0000E702}, NOP_C_("Nintendo3DSSysTitles", "Picture picker")},
	{{0x0000C402, 0x0000CC02, 0x0000D402, 0x0000DC02, 0x0000E202, 0x0000E802}, NOP_C_("Nintendo3DSSysTitles", "Voice memo picker")},
	{{0x0000C602, 0x0000CE02, 0x0000D602,          0, 0x0000E302, 0x0000E902}, NOP_C_("Nintendo3DSSysTitles", "eShop applet")},
	{{0x0000BC02, 0x0000BD02, 0x0000BE02,          0,          0,          0}, NOP_C_("Nintendo3DSSysTitles", "Miiverse")},
	{{0x00008302, 0x00008B02, 0x0000BA02,          0,          0,          0}, NOP_C_("Nintendo3DSSysTitles", "Miiverse posting applet")},
	{{0x00009502, 0x00009E02, 0x0000B902,          0, 0x00008C02, 0x0000BF02}, NOP_C_("Nintendo3DSSysTitles", "amiibo Settings")},

	/** New 3DS exclusive **/
	{{0x20008802, 0x20009402, 0x20009D02,          0, 0x2000AE02,          0}, NOP_C_("Nintendo3DSSysTitles", "Internet Browser")},
	{{0x2000C003, 0x2000C803, 0x2000D003,          0, 0x2000DE03,          0}, NOP_C_("Nintendo3DSSysTitles", "Software Keyboard (SAFE_MODE)")},
};

/** Nintendo3DSSysTitles **/

/**
 * Look up a Nintendo 3DS system title.
 * @param tid_hi	[in] Title ID High.
 * @param tid_lo	[in] Title ID Low.
 * @param pRegion	[out,opt] Output for region name.
 * @return System title name, or nullptr on error.
 */
const char *Nintendo3DSSysTitles::lookup_sys_title(uint32_t tid_hi, uint32_t tid_lo, const char **pRegion)
{
	const Nintendo3DSSysTitlesPrivate::SysTitle *titles;
	int title_count;

	if (tid_hi == 0 || tid_lo == 0 ||
	    tid_hi == 0xFFFFFFFF || tid_lo == 0xFFFFFFFF)
	{
		// Not supported.
		if (pRegion) {
			*pRegion = nullptr;
		}
		return nullptr;
	}

	if (tid_hi == 0x00040010) {
		titles = Nintendo3DSSysTitlesPrivate::sys_title_00040010;
		title_count = ARRAY_SIZE(Nintendo3DSSysTitlesPrivate::sys_title_00040010);
	} else if (tid_hi == 0x00040030) {
		titles = Nintendo3DSSysTitlesPrivate::sys_title_00040030;
		title_count = ARRAY_SIZE(Nintendo3DSSysTitlesPrivate::sys_title_00040030);
	} else {
		// tid_hi not supported.
		if (pRegion) {
			*pRegion = nullptr;
		}
		return nullptr;
	}

	// TODO: bsearch()?
	for (; title_count > 0; title_count--, titles++) {
		for (unsigned int region = 0; region < 6; region++) {
			if (titles->tid_lo[region] != 0 && titles->tid_lo[region] == tid_lo) {
				// Found a match!
				if (pRegion) {
					*pRegion = Nintendo3DSSysTitlesPrivate::regions[region];
				}
				return dpgettext_expr(RP_I18N_DOMAIN, "Nintendo3DSSysTitles", titles->desc);
			}
		}
	}

	// Not found.
	return nullptr;
}

}
