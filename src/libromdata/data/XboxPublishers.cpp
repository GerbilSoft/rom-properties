/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxPublishers.hpp: Xbox third-party publishers list.                   *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XboxPublishers.hpp"

namespace LibRomData {

class XboxPublishersPrivate {
	private:
		// Static class.
		XboxPublishersPrivate();
		~XboxPublishersPrivate();
		RP_DISABLE_COPY(XboxPublishersPrivate)

	public:
		struct ThirdPartyEntry {
			uint16_t code;			// 2-byte code
			const char *publisher;
		};

		/**
		 * Xbox third-party publisher list.
		 * This list is valid for Xbox and Xbox 360.
		 *
		 * References:
		 * - https://xboxdevwiki.net/Xbe
		 */
		static const ThirdPartyEntry thirdPartyList[];

		/**
		 * Comparison function for bsearch().
		 * For use with ThirdPartyEntry.
		 *
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API compar(const void *a, const void *b);

};

/**
 * Xbox third-party publisher list.
 * This list is valid for Xbox and Xbox 360
 *
 * References:
 * - https://xboxdevwiki.net/Xbe
 */
const XboxPublishersPrivate::ThirdPartyEntry XboxPublishersPrivate::thirdPartyList[] = {
	{0,	"<unlicensed>"},
	{'AC',	"Acclaim Entertainment"},
	{'AH',	"ARUSH Entertainment"},
	{'AQ',	"Aqua System"},
	{'AS',	"ASK"},
	{'AT',	"Atlus"},
	{'AV',	"Activision"},
	{'AY',	"Aspyr Media"},
	{'BA',	"Bandai"},
	{'BL',	"Black Box"},
	{'BM',	"BAM! Entertainment"},
	{'BR',	"Broccoli Co."},
	{'BS',	"Bethesda Softworks"},
	{'BU',	"Bunkasha Co."},
	{'BV',	"Buena Vista Games"},
	{'BW',	"BBC Multimedia"},
	{'BZ',	"Blizzard"},
	{'CC',	"Capcom"},
	{'CK',	"Kemco Corporation"},
	{'CM',	"Codemasters"},
	{'CV',	"Crave Entertainment"},
	{'DC',	"DreamCatcher Interactive"},
	{'DX',	"Davilex"},
	{'EA',	"Electronic Arts (EA)"},
	{'EC',	"Encore inc"},
	{'EL',	"Enlight Software"},
	{'EM',	"Empire Interactive"},
	{'ES',	"Eidos Interactive"},
	{'FI',	"Fox Interactive"},
	{'FS',	"From Software"},
	{'GE',	"Genki Co."},
	{'GV',	"Groove Games"},
	{'HE',	"Tru Blu"},
	{'HP',	"Hip games"},
	{'HU',	"Hudson Soft"},
	{'HW',	"Highwaystar"},
	{'IA',	"Mad Catz Interactive"},
	{'IF',	"Idea Factory"},
	{'IG',	"Infogrames"},
	{'IL',	"Interlex Corporation"},
	{'IM',	"Imagine Media"},
	{'IO',	"Ignition Entertainment"},
	{'IP',	"Interplay Entertainment"},
	{'IX',	"InXile Entertainment"},
	{'JA',	"Jaleco"},
	{'JW',	"JoWooD"},
	{'KB',	"Kemco"},
	{'KI',	"Kids Station Inc."},
	{'KN',	"Konami"},
	{'KO',	"KOEI"},
	{'KU',	"Kobi and/or GAE"},
	{'LA',	"LucasArts"},
	{'LS',	"Black Bean Games"},
	{'MD',	"Metro3D"},
	{'ME',	"Medix"},
	{'MI',	"Microïds"},
	{'MJ',	"Majesco Entertainment"},
	{'MM',	"Myelin Media"},
	{'MP',	"MediaQuest"},
	{'MS',	"Microsoft Game Studios"},
	{'MW',	"Midway Games"},
	{'MX',	"Empire Interactive"},
	{'NK',	"NewKidCo"},
	{'NL',	"NovaLogic"},
	{'NM',	"Namco"},
	{'OX',	"Oxygen Interactive"},
	{'PC',	"Playlogic Entertainment"},
	{'PL',	"Phantagram Co., Ltd."},
	{'RA',	"Rage"},
	{'SA',	"Sammy"},
	{'SC',	"SCi Games"},
	{'SE',	"SEGA"},
	{'SN',	"SNK"},
	{'SS',	"Simon & Schuster"},
	{'SU',	"Success Corporation"},
	{'SW',	"Swing! Deutschland"},
	{'TA',	"Takara"},
	{'TC',	"Tecmo"},
	{'TD',	"The 3DO Company"},
	{'TK',	"Takuyo"},
	{'TM',	"TDK Mediactive"},
	{'TQ',	"THQ"},
	{'TS',	"Titus Interactive"},
	{'TT',	"Take-Two Interactive Software"},
	{'US',	"Ubisoft"},
	{'VC',	"Victor Interactive Software"},
	{'VN',	"Vivendi Universal Games"},
	{'VU',	"Vivendi Universal Games"},
	{'VV',	"Vivendi Universal Games"},
	{'WE',	"Wanadoo Edition"},
	{'WR',	"Warner Bros. Interactive Entertainment"},
	{'XI',	"XPEC Entertainment and Idea Factory"},
	{'XK',	"Xbox kiosk disk"},
	{'XL',	"Xbox special bundled or live demo disk"},
	{'XM',	"Evolved Games"},
	{'XP',	"XPEC Entertainment"},
	{'XR',	"Panorama"},
	{'YB',	"YBM Sisa"},
	{'ZD',	"Zushi Games (Zoo Digital Publishing)"},

	{0, nullptr}
};

/**
 * Comparison function for bsearch().
 * For use with ThirdPartyEntry.
 *
 * @param a
 * @param b
 * @return
 */
int RP_C_API XboxPublishersPrivate::compar(const void *a, const void *b)
{
	uint16_t code1 = static_cast<const ThirdPartyEntry*>(a)->code;
	uint16_t code2 = static_cast<const ThirdPartyEntry*>(b)->code;
	if (code1 < code2) return -1;
	if (code1 > code2) return 1;
	return 0;
}

/** Public functions **/

/**
 * Look up a company code.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *XboxPublishers::lookup(uint16_t code)
{
	// Do a binary search.
	const XboxPublishersPrivate::ThirdPartyEntry key = {code, nullptr};
	const XboxPublishersPrivate::ThirdPartyEntry *res =
		static_cast<const XboxPublishersPrivate::ThirdPartyEntry*>(bsearch(&key,
			XboxPublishersPrivate::thirdPartyList,
			ARRAY_SIZE(XboxPublishersPrivate::thirdPartyList)-1,
			sizeof(XboxPublishersPrivate::ThirdPartyEntry),
			XboxPublishersPrivate::compar));
	return (res ? res->publisher : nullptr);
}

/**
 * Look up a company code.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *XboxPublishers::lookup(const char *code)
{
	const uint16_t code16 = (static_cast<uint8_t>(code[0]) << 8) |
				 static_cast<uint8_t>(code[1]);
	return lookup(code16);
}

}
