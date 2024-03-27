/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WonderSwanPublishers.hpp: Bandai WonderSwan third-party publishers list.*
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WonderSwanPublishers.hpp"

// C++ STL classes
using std::array;

namespace LibRomData { namespace WonderSwanPublishers {

struct ThirdPartyEntry {
	char code[4];		// 3-char code
	const char *publisher;	// Publisher name
};

/**
 * Bandai WonderSwan third-party publisher list.
 * Array index is the publisher ID.
 * Reference: http://daifukkat.su/docs/wsman/#cart_meta_publist
 */
static const array<ThirdPartyEntry, 0x41> thirdPartyList = {{
	// 0x00
	{"",	"<unlicensed>"},
	{"BAN",	"Bandai"},
	{"TAT",	"Taito"},
	{"TMY",	"Tomy"},
	{"KEX",	"Koei"},
	{"DTE",	"Data East"},
	{"AAE",	"Asmik Ace"},
	{"MDE", "Media Entertainment"},
	{"NHB",	"Nichibutsu"},
	{"",	nullptr},
	{"CCJ",	"Coconuts Japan"},
	{"SUM",	"Sammy"},
	{"SUN",	"Sunsoft"},
	{"PAW",	"Mebius"},	// (?)
	{"BPR",	"Banpresto"},
	{"",	nullptr},

	// 0x10
	{"JLC",	"Jaleco"},
	{"MGA",	"Imagineer"},
	{"KNM",	"Konami"},
	{"",	nullptr},
	{"",	nullptr},
	{"",	nullptr},
	{"KBS",	"Kobunsha"},
	{"BTM",	"Bottom Up"},
	{"KGT",	"Kaga Tech"},
	{"SRV",	"Sunrise"},
	{"CFT",	"Cyber Front"},
	{"MGH",	"Mega House"},
	{"",	nullptr},
	{"BEC",	"Interbec"},
	{"NAP",	"Nihon Application"},
	{"BVL",	"Bandai Visual"},

	// 0x20
	{"ATN",	"Athena"},
	{"KDX",	"KID"},
	{"HAL",	"HAL Corporation"},
	{"YKE",	"Yuki Enterprise"},
	{"OMM",	"Omega Micott"},
	{"LAY",	"Layup"},
	{"KDK",	"Kadokawa Shoten"},
	{"SHL",	"Shall Luck"},
	{"SQR",	"Squaresoft"},
	{"",	nullptr},
	{"SCC",	"NTT Docomo"},	// Mobile Wonder Gate
	{"TMC",	"Tom Create"},
	{"",	nullptr},
	{"NMC",	"Namco"},
	{"SES",	"Movic"},	// (?)
	{"HTR",	"E3 Staff"},	// (?)

	// 0x30
	{"",	nullptr},
	{"VGD",	"Vanguard"},
	{"MGT",	"Megatron"},
	{"WIZ",	"Wiz"},
	{"",	nullptr},
	{"",	nullptr},
	{"CAP",	"Capcom"},
	{"",	nullptr},
	{"",	nullptr},
	{"",	nullptr},
	{"",	nullptr},
	{"",	nullptr},
	{"",	nullptr},
	{"",	nullptr},
	{"",	nullptr},
	{"",	nullptr},

	// 0x40
	{"DDJ",	"Digital Dream"},	// FIXME: Not the actual publisher ID. (Cart has 0x00)
}};

/** Public functions **/

/**
 * Look up a company name.
 * @param id Company ID.
 * @return Publisher name, or nullptr if not found.
 */
const char *lookup_name(uint8_t id)
{
	if (id >= thirdPartyList.size())
		return nullptr;

	return thirdPartyList[id].publisher;
}

/**
 * Look up a company name.
 * @param id Company ID.
 * @return Publisher code, or nullptr if not found.
 */
const char *lookup_code(uint8_t id)
{
	if (id >= thirdPartyList.size())
		return nullptr;

	const char *code = thirdPartyList[id].code;
	return (code[0] != '\0' ? code : nullptr);
}

} }
