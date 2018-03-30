/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SegaPublishers.cpp: Sega third-party publishers list.                   *
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

#include "SegaPublishers.hpp"

// C includes.
#include <stdlib.h>

namespace LibRomData {

class SegaPublishersPrivate {
	private:
		// Static class.
		SegaPublishersPrivate();
		~SegaPublishersPrivate();
		RP_DISABLE_COPY(SegaPublishersPrivate)

	public:
		/**
		 * Sega third-party publisher list.
		 * Reference: http://segaretro.org/Third-party_T-series_codes
		 */
		struct TCodeEntry {
			unsigned int t_code;
			const char *publisher;
		};
		static const TCodeEntry tcodeList[];

		/**
		 * Comparison function for bsearch().
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API compar(const void *a, const void *b);

};

/**
 * Sega third-party publisher list.
 * Reference: http://segaretro.org/Third-party_T-series_codes
 */
const SegaPublishersPrivate::TCodeEntry SegaPublishersPrivate::tcodeList[] = {
	{0,	"Sega"},
	{11,	"Taito"},
	{12,	"Capcom"},
	{13,	"Data East"},
	{14,	"Namco (Namcot)"},
	{15,	"Sun Electronics (Sunsoft)"},
	{16,	"Ma-Ba"},
	{17,	"Dempa"},
	{18,	"Tecno Soft"},
	{19,	"Tecno Soft"},
	{20,	"Asmik"},
	{21,	"ASCII"},
	{22,	"Micronet"},
	{23,	"VIC Tokai"},
	{24,	"Treco, Sammy"},
	{25,	"Nippon Computer Systems (Masaya)"},
	{26,	"Sigma Enterprises"},
	{27,	"Toho"},
	{28,	"HOT-B"},
	{29,	"Kyugo"},
	{30,	"Video System"},
	{31,	"SNK"},
	{32,	"Wolf Team"},
	{33,	"Kaneko"},
	{34,	"DreamWorks"},
	{35,	"Seismic Software"},
	{36,	"Tecmo"},
	{38,	"Mediagenic"},
	{40,	"Toaplan"},
	{41,	"UNIPACC"},
	{42,	"UPL"},
	{43,	"Human"},
	{44,	"Sanritsu (SIMS)"},
	{45,	"Game Arts"},
	{46,	"Kodansha Research Institute"},
	{47,	"Sage's Creation"},
	{48,	"Tengen (Time Warner Interactive)"},
	{49,	"Telenet Japan, Micro World"},
	{50,	"Electronic Arts"},
	{51,	"Microcabin"},
	{52,	"SystemSoft (SystemSoft Alpha)"},
	{53,	"Riverhillsoft"},
	{54,	"Face"},
	{55,	"Nuvision Entertainment"},
	{56,	"Razorsoft"},
	{57,	"Jaleco"},
	{58,	"Visco"},
	{60,	"Victor Musical Industries (Victor Entertainment, Victor Soft)"},
	{61,	"Toyo Recording Co. (Wonder Amusement Studio)"},
	{62,	"Sony Imagesoft"},
	{63,	"Toshiba EMI"},
	{64,	"Information Global Service"},
	{65,	"Tsukuda Ideal"},
	{66,	"Compile"},
	{67,	"Home Data (Magical)"},
	{68,	"CSK Research Institute (CRI)"},
	{69,	"Arena Entertainment"},
	{70,	"Virgin Interactive"},
	{71,	"Nihon Bussan (Nichibutsu)"},
	{72,	"Varie"},
	{73,	"Coconuts Japan, Soft Vision"},
	{74,	"PALSOFT"},
	{75,	"Pony Canyon"},
	{76,	"Koei"},
	{77,	"Takeru (Sur De Wave)"},
	{79,	"U.S. Gold"},
	{81,	"Acclaim Entertainment, Flying Edge"},
	{83,	"GameTek"},
	{84,	"Datawest"},
	{85,	"PCM Complete"},
	{86,	"Absolute Entertainment"},
	{87,	"Mindscape (The Software Toolworks)"},
	{88,	"Domark"},
	{89,	"Parker Brothers"},
	{91,	"Pack-In Video (Victor Interactive Software, Pack-In-Soft, Victor Soft)"},
	{92,	"Polydor (Sandstorm)"},
	{93,	"Sony"},
	{95,	"Konami"},
	{97,	"Tradewest, Williams Entertainment, Midway Games"},
	{99,	"Success"},
	{100,	"THQ, Black Pearl Software)"},
	{101,	"TecMagik Entertainment"},
	{102,	"Samsung"},
	{103,	"Takara"},
	{105,	"Shogakukan Production"},
	{106,	"Electronic Arts Victor"},
	{107,	"Electro Brain"},
	{109,	"Saddleback Graphics"},
	{110,	"Dynamix (Simon & Schuster Interactive)"},
	{111,	"American Laser Games"},
	{112,	"Hi-Tech Expressions"},
	{113,	"Psygnosis"},
	{114,	"T&E Soft"},
	{115,	"Core Design"},
	{118,	"The Learning Company"},
	{119,	"Accolade"},
	{120,	"Codemasters"},
	{121,	"ReadySoft"},
	{123,	"Gremlin Interactive"},
	{124,	"Spectrum Holobyte"},
	{125,	"Interplay"},
	{126,	"Maxis"},
	{127,	"Working Designs"},
	{130,	"Activision"},
	{132,	"Playmates Interactive Entertainment"},
	{133,	"Bandai"},
	{135,	"CapDisc"},
	{137,	"ASC Games"},
	{139,	"Viacom New Media"},
	{141,	"Toei Video"},
	{143,	"Hudson (Hudson Soft)"},
	{144,	"Atlus"},
	{145,	"Sony Music Entertainment"},
	{146,	"Takara"},
	{147,	"Sansan"},
	{149,	"Nisshouiwai Infocom"},
	{150,	"Imagineer (Imadio)"},
	{151,	"Infogrames"},
	{152,	"Davidson & Associates"},
	{153,	"Rocket Science Games"},
	{154,	"Technōs Japan"},
	{157,	"Angel"},
	{158,	"Mindscape"},
	{159,	"Crystal Dynamics"},
	{160,	"Sales Curve Interactive"},
	{161,	"Fox Interactive"},
	{162,	"Digital Pictures"},
	{164,	"Ocean Software"},
	{165,	"Seta"},
	{166,	"Altron"},
	{167,	"ASK Kodansha"},
	{168,	"Athena"},
	{169,	"Gakken"},
	{170,	"General Entertainment"},
	{172,	"EA Sports"},
	{174,	"Glams"},
	{176,	"ASCII Something Good"},
	{177,	"Ubisoft"},
	{178,	"Hitachi"},
	{180,	"BMG Interactive Entertainment (BMG Victor, BMG Japan)"},
	{181,	"Obunsha"},
	{182,	"Thinking Cap"},
	{185,	"Gaga Communications"},
	{186,	"SoftBank (Game Bank)"},
	{187,	"Naxat Soft (Pionesoft)"},
	{188,	"Mizuki (Spike, Maxbet)"},
	{189,	"KAZe"},
	{193,	"Sega Yonezawa"},
	{194,	"We Net"},
	{195,	"Datam Polystar"},
	{197,	"KID"},
	{198,	"Epoch"},
	{199,	"Ving"},
	{200,	"Yoshimoto Kogyo"},
	{201,	"NEC Interchannel (InterChannel)"},
	{202,	"Sonnet Computer Entertainment"},
	{203,	"Game Studio"},
	{204,	"Psikyo"},
	{205,	"Media Entertainment"},
	{206,	"Banpresto"},
	{207,	"Ecseco Development"},
	{208,	"Bullet-Proof Software (BPS)"},
	{209,	"Sieg"},
	{210,	"Yanoman"},
	{212,	"Oz Club"},
	{213,	"Nihon Create"},
	{214,	"Media Rings Corporation"},
	{215,	"Shoeisha"},
	{216,	"OPeNBooK"},
	{217,	"Hakuhodo (Hamlet)"},
	{218,	"Aroma (Yumedia)"},
	{219,	"Societa Daikanyama"},
	{220,	"Arc System Works"},
	{221,	"Climax Entertainment"},
	{222,	"Pioneer LDC"},
	{223,	"Tokuma Shoten"},
	{224,	"I'MAX"},
	{226,	"Shogakukan"},
	{227,	"Vantan International"},
	{229,	"Titus"},
	{230,	"LucasArts"},
	{231,	"Pai"},
	{232,	"Ecole (Reindeer)"},
	{233,	"Nayuta"},
	{234,	"Bandai Visual"},
	{235,	"Quintet"},
	{239,	"Disney Interactive"},
	{240,	"9003 (OpenBook9003)"},
	{241,	"Multisoft"},
	{242,	"Sky Think System"},
	{243,	"OCC"},
	{246,	"Increment P (iPC)"},
	{249,	"King Records"},
	{250,	"Fun House"},
	{251,	"Patra"},
	{252,	"Inner Brain"},
	{253,	"Make Software"},
	{254,	"GT Interactive Software"},
	{255,	"Kodansha"},
	{257,	"Clef"},
	{259,	"C-Seven"},
	{260,	"Fujitsu Parex"},
	{261,	"Xing Entertainment"},
	{264,	"Media Quest"},
	{268,	"Wooyoung System"},
	{270,	"Nihon System"},
	{271,	"Scholar"},
	{273,	"Datt Japan"},
	{278,	"MediaWorks"},
	{279,	"Kadokawa Shoten"},
	{280,	"Elf"},
	{282,	"Tomy"},
	{289,	"KSS"},
	{290,	"Mainichi Communications"},
	{291,	"Warashi"},
	{292,	"Metro"},
	{293,	"Sai-Mate"},
	{294,	"Kokopeli Digital Studios"},
	{296,	"Planning Office Wada (POW)"},
	{297,	"Telstar"},
	{300,	"Warp, Kumon Publishing"},
	{303,	"Masudaya"},
	{306,	"Soft Office"},
	{307,	"Empire Interactive"},
	{308,	"Genki (Sada Soft)"},
	{309,	"Neverland"},
	{310,	"Shar Rock"},
	{311,	"Natsume"},
	{312,	"Nexus Interact"},
	{313,	"Aplix Corporation"},
	{314,	"Omiya Soft"},
	{315,	"JVC"},
	{316,	"Zoom"},
	{321,	"TEN Institute"},
	{322,	"Fujitsu"},
	{325,	"TGL"},
	{326,	"Red Company (Red Entertainment)"},
	{328,	"Waka Manufacturing"},
	{329,	"Treasure"},
	{330,	"Tokuma Shoten Intermedia"},
	{331,	"Sonic! Software Planning (Camelot)"},
	{339,	"Sting"},
	{340,	"Chunsoft"},
	{341,	"Aki"},
	{342,	"From Software"},
	{346,	"Daiki"},
	{348,	"Aspect"},
	{350,	"Micro Vision"},
	{351,	"Gainax"},
	{354,	"FortyFive (45XLV)"},
	{355,	"Enix"},
	{356,	"Ray Corporation"},
	{357,	"Tonkin House"},
	{360,	"Outrigger"},
	{361,	"B-Factory"},
	{362,	"LayUp"},
	{363,	"Axela"},
	{364,	"WorkJam"},
	{365,	"Nihon Syscom (Syscom Entertainment)"},
	{367,	"FOG (Full On Games)"},
	{368,	"Eidos Interactive"},
	{369,	"UEP Systems"},
	{370,	"Shouei System"},
	{371,	"GMF"},
	{373,	"ADK"},
	{374,	"Softstar Entertainment"},
	{375,	"Nexton"},
	{376,	"Denshi Media Services"},
	{379,	"Takuyo"},
	{380,	"Starlight Marry"},
	{381,	"Crystal Vision"},
	{382,	"Kamata and Partners"},
	{383,	"AquaPlus"},
	{384,	"Media Gallop"},
	{385,	"Culture Brain"},
	{386,	"Locus"},
	{387,	"Entertainment Software Publishing (ESP)"},
	{388,	"NEC Home Electronics"},
	{390,	"Pulse Interactive"},
	{391,	"Random House"},
	{394,	"Vivarium"},
	{395,	"Mebius"},
	{396,	"Panther Software"},
	{397,	"TBS"},
	{398,	"NetVillage (Gamevillage)"},
	{400,	"Vision (Noisia)"},
	{401,	"Shangri-La"},
	{402,	"Crave Entertainment"},
	{403,	"Metro3D"},
	{404,	"Majesco"},
	{405,	"Take-Two Interactive"},
	{406,	"Hasbro Interactive"},
	{407,	"Rage Software (Rage Games)"},
	{408,	"Marvelous Entertainment"},
	{409,	"Bottom Up"},
	{410,	"Daikoku Denki"},
	{411,	"Sunrise Interactive"},
	{412,	"Bimboosoft"},
	{413,	"UFO"},
	{414,	"Mattel Interactive"},
	{415,	"CaramelPot"},
	{416,	"Vatical Entertainment"},
	{417,	"Ripcord Games"},
	{418,	"Sega Toys"},
	{419,	"Gathering of Developers"},
	{421,	"Rockstar Games"},
	{422,	"Winkysoft"},
	{423,	"Cyberfront"},
	{424,	"DaZZ"},
	{428,	"Kobi"},
	{429,	"Fujicom"},
	{433,	"Real Vision"},
	{434,	"Visit"},
	{435,	"Global A Entertainment"},
	{438,	"Studio Wonder Effect"},
	{439,	"Media Factory"},
	{441,	"Red"},
	{443,	"Agetec"},
	{444,	"Abel"},
	{445,	"Softmax"},
	{446,	"Isao"},
	{447,	"Kool Kizz"},
	{448,	"GeneX"},
	{449,	"Xicat Interactive"},
	{450,	"Swing! Entertainment"},
	{451,	"Yuke's"},
	{454,	"AAA Game"},
	{455,	"TV Asahi"},
	{456,	"Crazy Games"},
	{457,	"Atmark"},
	{458,	"Hackberry"},
	{460,	"AIA"},
	{461,	"Starfish-SD"},
	{462,	"Idea Factory"},
	{463,	"Broccoli"},
	{465,	"Oaks (Princess Soft)"},
	{466,	"Bigben Interactive"},
	{467,	"G.rev"},
	{469,	"Symbio Planning"},
	{471,	"Alchemist"},
	{473,	"SNK Playmore"},
	{474,	"D3Publisher"},
	{475,	"Rain Software (Charara)"},
	{476,	"Good Navigate (GN Software)"},
	{477,	"Alfa System"},
	{478,	"Milestone Inc."},
	{479,	"Triangle Service"},

	{0, nullptr}
};

/**
 * Comparison function for bsearch().
 * @param a
 * @param b
 * @return
 */
int RP_C_API SegaPublishersPrivate::compar(const void *a, const void *b)
{
	unsigned int code1 = static_cast<const TCodeEntry*>(a)->t_code;
	unsigned int code2 = static_cast<const TCodeEntry*>(b)->t_code;
	if (code1 < code2) return -1;
	if (code1 > code2) return 1;
	return 0;
}

/**
 * Look up a company code.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *SegaPublishers::lookup(unsigned int code)
{
	// Do a binary search.
	const SegaPublishersPrivate::TCodeEntry key = {code, nullptr};
	const SegaPublishersPrivate::TCodeEntry *res =
		static_cast<const SegaPublishersPrivate::TCodeEntry*>(bsearch(&key,
			SegaPublishersPrivate::tcodeList,
			ARRAY_SIZE(SegaPublishersPrivate::tcodeList)-1,
			sizeof(SegaPublishersPrivate::TCodeEntry),
			SegaPublishersPrivate::compar));
	return (res ? res->publisher : nullptr);
}

}
