/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WonderSwanPublishers.hpp: Bandai WonderSwan third-party publishers list.*
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WonderSwanPublishers.hpp"

namespace LibRomData { namespace WonderSwanPublishers {

struct ThirdPartyEntry {
	char code[4];		// 3-char code
	const char8_t *publisher;	// Publisher name
};

/**
 * Bandai WonderSwan third-party publisher list.
 * Array index is the publisher ID.
 * Reference: http://daifukkat.su/docs/wsman/#cart_meta_publist
 */
static const ThirdPartyEntry thirdPartyList[] = {
	// 0x00
	{"",	U8("<unlicensed>")},
	{"BAN",	U8("Bandai")},
	{"TAT",	U8("Taito")},
	{"TMY",	U8("Tomy")},
	{"KEX",	U8("Koei")},
	{"DTE",	U8("Data East")},
	{"AAE",	U8("Asmik Ace")},
	{"MDE",	U8("Media Entertainment")},
	{"NHB",	U8("Nichibutsu")},
	{"",	nullptr},
	{"CCJ",	U8("Coconuts Japan")},
	{"SUM",	U8("Sammy")},
	{"SUN",	U8("Sunsoft")},
	{"PAW",	U8("Mebius")},	// (?)
	{"BPR",	U8("Banpresto")},
	{"",	nullptr},

	// 0x10
	{"JLC",	U8("Jaleco")},
	{"MGA",	U8("Imagineer")},
	{"KNM",	U8("Konami")},
	{"",	nullptr},
	{"",	nullptr},
	{"",	nullptr},
	{"KBS",	U8("Kobunsha")},
	{"BTM",	U8("Bottom Up")},
	{"KGT",	U8("Kaga Tech")},
	{"SRV",	U8("Sunrise")},
	{"CFT",	U8("Cyber Front")},
	{"MGH",	U8("Mega House")},
	{"",	nullptr},
	{"BEC",	U8("Interbec")},
	{"NAP",	U8("Nihon Application")},
	{"BVL",	U8("Bandai Visual")},

	// 0x20
	{"ATN",	U8("Athena")},
	{"KDX",	U8("KID")},
	{"HAL",	U8("HAL Corporation")},
	{"YKE",	U8("Yuki Enterprise")},
	{"OMM",	U8("Omega Micott")},
	{"LAY",	U8("Layup")},
	{"KDK",	U8("Kadokawa Shoten")},
	{"SHL",	U8("Shall Luck")},
	{"SQR",	U8("Squaresoft")},
	{"",	nullptr},
	{"SCC",	U8("NTT Docomo")},	// Mobile Wonder Gate
	{"TMC",	U8("Tom Create")},
	{"",	nullptr},
	{"NMC",	U8("Namco")},
	{"SES",	U8("Movic")},	// (?)
	{"HTR",	U8("E3 Staff")},	// (?)

	// 0x30
	{"",	nullptr},
	{"VGD",	U8("Vanguard")},
	{"MGT",	U8("Megatron")},
	{"WIZ",	U8("Wiz")},
	{"",	nullptr},
	{"",	nullptr},
	{"CAP",	U8("Capcom")},
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
	{"DDJ",	U8("Digital Dream")},	// FIXME: Not the actual publisher ID. (Cart has 0x00)
};

/** Public functions **/

/**
 * Look up a company name.
 * @param id Company ID.
 * @return Publisher name, or nullptr if not found.
 */
const char8_t *lookup_name(uint8_t id)
{
	if (id >= ARRAY_SIZE(thirdPartyList))
		return nullptr;

	return thirdPartyList[id].publisher;
}

/**
 * Look up a company code.
 * @param id Company ID.
 * @return Publisher code, or nullptr if not found.
 */
const char *lookup_code(uint8_t id)
{
	if (id >= ARRAY_SIZE(thirdPartyList))
		return nullptr;

	const char *code = thirdPartyList[id].code;
	return (code[0] != '\0') ? code : nullptr;
}

} }
