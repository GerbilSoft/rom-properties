/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoPublishers.cpp: Nintendo third-party publishers list.           *
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

#include "NintendoPublishers.hpp"

// C includes.
#include <stdlib.h>

namespace LibRomData {

class NintendoPublishersPrivate {
	private:
		// Static class.
		NintendoPublishersPrivate();
		~NintendoPublishersPrivate();
		RP_DISABLE_COPY(NintendoPublishersPrivate)

	public:
		struct ThirdPartyEntry {
			uint16_t code;			// 2-byte code
			const char *publisher;
		};

		/**
		 * Nintendo third-party publisher list.
		 * This list is valid for most Nintendo systems.
		 *
		 * References:
		 * - http://www.gametdb.com/Wii
		 * - http://www.gametdb.com/Wii/Downloads
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

	public:
		struct ThirdPartyEntry_fds {
			uint8_t code;			// Old publisher code
			const char *publisher_en;
			const char *publisher_jp;
		};

		/**
		 * Nintendo third-party publisher list.
		 * This list is valid for Famicom Disk System only.
		 *
		 * References:
		 * - https://wiki.nesdev.com/w/index.php/Family_Computer_Disk_System#Manufacturer_codes
		 */
		static const ThirdPartyEntry_fds thirdPartyList_fds[];

		/**
		 * Comparison function for bsearch().
		 * For use with ThirdPartyEntry_fds.
		 *
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API compar_fds(const void *a, const void *b);
};

/**
 * Nintendo third-party publisher list.
 * This list is valid for most Nintendo systems.
 *
 * References:
 * - http://www.gametdb.com/Wii
 * - http://www.gametdb.com/Wii/Downloads
 * - https://wiki.nesdev.com/w/index.php/Family_Computer_Disk_System#Manufacturer_codes
 */
const NintendoPublishersPrivate::ThirdPartyEntry NintendoPublishersPrivate::thirdPartyList[] = {
	{'00',	"<unlicensed>"},
	{'01',	"Nintendo"},
	{'02',	"Rocket Games / Ajinomoto"},
	{'03',	"Imagineer-Zoom"},
	{'04',	"Gray Matter"},
	{'05',	"Zamuse"},
	{'06',	"Falcom"},
	{'07',	"Enix"},
	{'08',	"Capcom"},
	{'09',	"Hot B Co."},
	{'0A',	"Jaleco"},
	{'0B',	"Coconuts Japan"},
	{'0C',	"Coconuts Japan / G.X.Media"},
	{'0D',	"Micronet"},
	{'0E',	"Technos"},
	{'0F',	"Mebio Software"},
	{'0G',	"Shouei System"},
	{'0H',	"Starfish"},
	{'0J',	"Mitsui Fudosan / Dentsu"},
	{'0L',	"Warashi Inc."},
	{'0N',	"Nowpro"},
	{'0P',	"Game Village"},
	{'0Q',	"IE Institute"},
	{'12',	"Infocom"},
	{'13',	"Electronic Arts Japan"},
	{'15',	"Cobra Team"},
	{'16',	"Human / Field"},
	{'17',	"KOEI"},
	{'18',	"Hudson Soft"},
	{'19',	"S.C.P."},
	{'1A',	"Yanoman"},
	{'1C',	"Tecmo Products"},
	{'1D',	"Japan Glary Business"},
	{'1E',	"Forum / OpenSystem"},
	{'1F',	"Virgin Games (Japan)"},
	{'1G',	"SMDE"},
	{'1J',	"Daikokudenki"},
	{'1P',	"Creatures Inc."},
	{'1Q',	"TDK Deep Impresion"},
	{'20',	"Zoo"},
	{'21',	"Sunsoft / Tokai Engineering"},
	{'22',	"POW (Planning Office Wada) / VR1 Japan"},
	{'23',	"Micro World"},
	{'25',	"San-X"},
	{'26',	"Enix"},
	{'27',	"Loriciel / Electro Brain"},
	{'28',	"Kemco Japan"},
	{'29',	"Seta"},
	{'2A',	"Culture Brain"},
	{'2C',	"Palsoft"},
	{'2D',	"Visit Co.,Ltd."},
	{'2E',	"Intec"},
	{'2F',	"System Sacom"},
	{'2G',	"Poppo"},
	{'2H',	"Ubisoft Japan"},
	{'2J',	"Media Works"},
	{'2K',	"NEC InterChannel"},
	{'2L',	"Tam"},
	{'2M',	"Jordan"},
	{'2N',	"Smilesoft / Rocket"},
	{'2Q',	"Mediakite"},
	{'30',	"Viacom"},
	{'31',	"Carrozzeria"},
	{'32',	"Dynamic"},
	{'34',	"Magifact"},
	{'35',	"Hect"},
	{'36',	"Codemasters"},
	{'37',	"Taito / GAGA Communications"},
	{'38',	"Laguna"},
	{'39',	"Telstar / Event / Taito"},
	{'3A',	"Soedesco"},
	{'3B',	"Arcade Zone Ltd"},
	{'3C',	"Entertainment International / Empire Software"},
	{'3D',	"Loriciel"},
	{'3E',	"Gremlin Graphics"},
	{'3F',	"K.Amusement Leasing Co."},
	{'40',	"Seika Corp."},
	{'41',	"Ubi Soft Entertainment"},
	{'42',	"Sunsoft US"},
	{'44',	"Life Fitness"},
	{'46',	"System 3"},
	{'47',	"Spectrum Holobyte"},
	{'49',	"Irem"},
	{'4A',	"Gakken"},	// FDS
	{'4B',	"Raya Systems"},
	{'4C',	"Renovation Products"},
	{'4D',	"Malibu Games"},
	{'4F',	"Eidos"},
	{'4G',	"Playmates Interactive"},
	{'4J',	"Fox Interactive"},
	{'4K',	"Time Warner Interactive"},
	{'4Q',	"Disney Interactive"},
	{'4S',	"Black Pearl"},
	{'4U',	"Advanced Productions"},
	{'4X',	"GT Interactive"},
	{'4Y',	"RARE"},
	{'4Z',	"Crave Entertainment"},
	{'50',	"Absolute Entertainment"},
	{'51',	"Acclaim"},
	{'52',	"Activision"},
	{'53',	"American Sammy"},
	{'54',	"Take 2 Interactive / GameTek"},
	{'55',	"Hi Tech"},
	{'56',	"LJN LTD."},
	{'58',	"Mattel"},
	{'5A',	"Mindscape / Red Orb Entertainment"},
	{'5B',	"Romstar"},
	{'5C',	"Taxan"},
	{'5D',	"Midway / Tradewest"},
	{'5F',	"American Softworks"},
	{'5G',	"Majesco Sales Inc"},
	{'5H',	"3DO"},
	{'5K',	"Hasbro"},
	{'5L',	"NewKidCo"},
	{'5M',	"Telegames"},
	{'5N',	"Metro3D"},
	{'5P',	"Vatical Entertainment"},
	{'5Q',	"LEGO Media"},
	{'5S',	"Xicat Interactive"},
	{'5T',	"Cryo Interactive"},
	{'5W',	"Red Storm Entertainment"},
	{'5X',	"Microids"},
	{'5Z',	"Data Design / Conspiracy / Swing"},
	{'60',	"Titus"},
	{'61',	"Virgin Interactive"},
	{'62',	"Maxis"},
	{'64',	"LucasArts Entertainment"},
	{'67',	"Ocean"},
	{'68',	"Bethesda Softworks"},
	{'69',	"Electronic Arts"},
	{'6B',	"Laser Beam"},
	{'6E',	"Elite Systems"},
	{'6F',	"Electro Brain"},
	{'6G',	"The Learning Company"},
	{'6H',	"BBC"},
	{'6J',	"Software 2000"},
	{'6K',	"UFO Interactive Games"},
	{'6L',	"BAM! Entertainment"},
	{'6M',	"Studio 3"},
	{'6Q',	"Classified Games"},
	{'6S',	"TDK Mediactive"},
	{'6U',	"DreamCatcher"},
	{'6V',	"JoWood Produtions"},
	{'6W',	"Sega"},
	{'6X',	"Wannado Edition"},
	{'6Y',	"LSP (Light & Shadow Prod.)"},
	{'6Z',	"ITE Media"},
	{'70',	"Atari (Infogrames)"},
	{'71',	"Interplay"},
	{'72',	"JVC (US)"},
	{'73',	"Parker Brothers"},
	{'75',	"Sales Curve (Storm / SCI)"},
	{'78',	"THQ"},
	{'79',	"Accolade"},
	{'7A',	"Triffix Entertainment"},
	{'7C',	"Microprose Software"},
	{'7D',	"Sierra / Universal Interactive"},
	{'7F',	"Kemco"},
	{'7G',	"Rage Software"},
	{'7H',	"Encore"},
	{'7J',	"Zoo"},
	{'7K',	"Kiddinx"},
	{'7L',	"Simon & Schuster Interactive"},
	{'7M',	"Asmik Ace Entertainment Inc."},
	{'7N',	"Empire Interactive"},
	{'7Q',	"Jester Interactive"},
	{'7S',	"Rockstar Games"},
	{'7T',	"Scholastic"},
	{'7U',	"Ignition Entertainment"},
	{'7V',	"Summitsoft"},
	{'7W',	"Stadlbauer"},
	{'80',	"Misawa"},
	{'81',	"Teichiku"},
	{'82',	"Namco Ltd."},
	{'83',	"LOZC"},
	{'84',	"KOEI"},
	{'86',	"Tokuma Shoten Intermedia"},
	{'87',	"Tsukuda Original"},
	{'88',	"DATAM-Polystar"},
	{'8B',	"BulletProof Software (BPS)"},
	{'8C',	"Vic Tokai Inc."},
	{'8E',	"Character Soft"},
	{'8F',	"I'Max"},
	{'8G',	"Saurus"},
	{'8J',	"General Entertainment"},
	{'8N',	"Success"},
	{'8P',	"Sega Japan"},
	{'90',	"Takara Amusement"},
	{'91',	"Chun Soft"},
	{'92',	"Video System /  Mc O' River"},
	{'93',	"BEC"},
	{'95',	"Varie"},
	{'96',	"Yonezawa / S'pal"},
	{'97',	"Kaneko"},
	{'99',	"Marvelous Entertainment"},
	{'9A',	"Nichibutsu / Nihon Bussan"},
	{'9B',	"Tecmo"},
	{'9C',	"Imagineer"},
	{'9F',	"Nova"},
	{'9G',	"Take2 / Den'Z / Global Star"},
	{'9H',	"Bottom Up"},
	{'9J',	"TGL (Technical Group Laboratory)"},
	{'9L',	"Hasbro Japan"},
	{'9N',	"Marvelous Entertainment"},
	{'9P',	"Keynet Inc."},
	{'9Q',	"Hands-On Entertainment"},
	{'A0',	"Telenet"},
	{'A1',	"Hori"},
	{'A2',	"Scorpion Soft"},	// FDS
	{'A4',	"Konami"},
	{'A5',	"K.Amusement Leasing Co."},
	{'A6',	"Kawada Co., Ltd."},
	{'A7',	"Takara"},
	{'A8',	"Royal Industries"},	// FDS
	{'A9',	"Technos Japan Corp."},
	{'AA',	"JVC / Victor"},
	{'AC',	"Toei Animation"},
	{'AD',	"Toho"},
	{'AF',	"Namco"},
	{'AG',	"Media Rings Corporation"},
	{'AH',	"J-Wing"},
	{'AJ',	"Pioneer LDC"},
	{'AK',	"KID"},
	{'AL',	"Mediafactory"},
	{'AP',	"Infogrames / Hudson"},
	{'AQ',	"Kiratto. Ludic Inc"},
	{'AY',	"Yacht Club Games"},
	{'B0',	"Acclaim Japan"},
	{'B1',	"ASCII Corporation"},
	{'B2',	"Bandai"},
	{'B3',	"Soft Pro Inc."},	// FDS
	{'B4',	"Enix"},
	{'B6',	"HAL Laboratory"},
	{'B7',	"SNK"},
	{'B9',	"Pony Canyon"},
	{'BA',	"Culture Brain"},
	{'BB',	"Sunsoft"},
	{'BC',	"Toshiba EMI"},
	{'BD',	"Sony Imagesoft"},
	{'BF',	"Sammy"},
	{'BG',	"Magical"},
	{'BH',	"Visco"},
	{'BJ',	"Compile"},
	{'BL',	"MTO Inc."},
	{'BN',	"Sunrise Interactive"},
	{'BP',	"Global A Entertainment"},
	{'BQ',	"Fuuki"},
	{'C0',	"Taito"},
	{'C1',	"Sunsoft / Ask Co., Ltd."},	// FDS
	{'C2',	"Kemco"},
	{'C3',	"Square"},
	{'C4',	"Tokuma Shoten"},
	{'C5',	"Data East"},
	{'C6',	"Tonkin House / Tokyo Shoseki"},
	{'C7',	"East Cube"},	// FDS
	{'C8',	"Koei"},
	{'CA',	"Konami / Ultra / Palcom"},
	{'CB',	"NTVIC / VAP"},
	{'CC',	"Use Co., Ltd."},
	{'CD',	"Meldac"},
	{'CE',	"Pony Canyon / FCI"},
	{'CF',	"Angel / Sotsu Agency / Sunrise"},
	{'CG',	"Yumedia / Aroma Co., Ltd"},
	{'CJ',	"Boss"},
	{'CK',	"Axela / Crea-Tech"},
	{'CL',	"Sekaibunka-Sha / Sumire Kobo / Marigul Management Inc."},
	{'CM',	"Konami Computer Entertainment Osaka"},
	{'CN',	"NEC Interchannel"},
	{'CP',	"Enterbrain"},
	{'CQ',	"From Software"},
	{'D0',	"Taito / Disco"},
	{'D1',	"Sofel"},
	{'D2',	"Quest / Bothtec"},
	{'D3',	"Sigma"},
	{'D4',	"Ask Kodansha"},
	{'D6',	"Naxat"},
	{'D7',	"Copya System"},
	{'D8',	"Capcom Co., Ltd."},
	{'D9',	"Banpresto"},
	{'DA',	"Tomy"},
	{'DB',	"LJN Japan"},
	{'DD',	"NCS"},
	{'DE',	"Human Entertainment"},
	{'DF',	"Altron"},
	{'DG',	"Jaleco"},
	{'DH',	"Gaps Inc."},
	{'DN',	"Elf"},
	{'DQ',	"Compile Heart"},
	{'DV',	"FarSight Studios"},
	{'E0',	"Jaleco"},
	{'E2',	"Yutaka"},
	{'E3',	"Varie"},
	{'E4',	"T&ESoft"},
	{'E5',	"Epoch"},
	{'E7',	"Athena"},
	{'E8',	"Asmik"},
	{'E9',	"Natsume"},
	{'EA',	"King Records"},
	{'EB',	"Atlus"},
	{'EC',	"Epic / Sony Records"},
	{'EE',	"IGS (Information Global Service)"},
	{'EG',	"Chatnoir"},
	{'EH',	"Right Stuff"},
	{'EL',	"Spike"},
	{'EM',	"Konami Computer Entertainment Tokyo"},
	{'EN',	"Alphadream Corporation"},
	{'EP',	"Sting"},
	{'ES',	"Star-Fish"},
	{'F0',	"A Wave"},
	{'F1',	"Motown Software"},
	{'F2',	"Left Field Entertainment"},
	{'F3',	"Extreme Ent. Grp."},
	{'F4',	"TecMagik"},
	{'F9',	"Cybersoft"},
	{'FB',	"Psygnosis"},
	{'FE',	"Davidson / Western Tech."},
	{'FK',	"The Game Factory"},
	{'FL',	"Hip Games"},
	{'FM',	"Aspyr"},
	{'FP',	"Mastiff"},
	{'FQ',	"iQue"},
	{'FR',	"Digital Tainment Pool"},
	{'FS',	"XS Games / Jack Of All Games"},
	{'FT',	"Daiwon"},
	{'G0',	"Alpha Unit"},
	{'G1',	"PCCW Japan"},
	{'G2',	"Yuke's Media Creations"},
	{'G4',	"KiKi Co Ltd"},
	{'G5',	"Open Sesame Inc"},
	{'G6',	"Sims"},
	{'G7',	"Broccoli"},
	{'G8',	"Avex"},
	{'G9',	"D3 Publisher"},
	{'GB',	"Konami Computer Entertainment Japan"},
	{'GD',	"Square-Enix"},
	{'GE',	"KSG"},
	{'GF',	"Micott & Basara Inc."},
	{'GG',	"O3 Entertainment"},
	{'GH',	"Orbital Media"},
	{'GJ',	"Detn8 Games"},
	{'GL',	"Gameloft / Ubi Soft"},
	{'GM',	"Gamecock Media Group"},
	{'GN',	"Oxygen Games"},
	{'GT',	"505 Games"},
	{'GY',	"The Game Factory"},
	{'H1',	"Treasure"},
	{'H2',	"Aruze"},
	{'H3',	"Ertain"},
	{'H4',	"SNK Playmore"},
	{'HF',	"Level-5"},
	{'HJ',	"Genius Products"},
	{'HY',	"Reef Entertainment"},
	{'HZ',	"Nordcurrent"},
	{'IH',	"Yojigen"},
	{'J9',	"AQ Interactive"},
	{'JF',	"Arc System Works"},
	{'JJ',	"Deep Silver"},
	{'JW',	"Atari"},
	{'K6',	"Nihon System"},
	{'KB',	"NIS America"},
	{'KM',	"Deep Silver"},
	{'KP',	"Purple Hills"},
	{'LH',	"Trend Verlag / East Entertainment"},
	{'LT',	"Legacy Interactive"},
	{'ME',	"SilverStar Games"},
	{'MJ',	"Mumbo Jumbo"},
	{'MR',	"Mindscape"},
	{'MS',	"Milestone / UFO Interactive"},
	{'MT',	"Blast !"},
	{'N9',	"Terabox"},
	{'NG',	"Nordic Games"},
	{'NK',	"Neko Entertainment / Diffusion / Naps team"},
	{'NP',	"Nobilis"},
	{'NQ',	"Namco Bandai"},
	{'NR',	"Data Design / Destineer Studios"},
	{'NS',	"NIS America"},
	{'PG',	"Phoenix Games"},
	{'PL',	"Playlogic"},
	{'RM',	"Rondomedia"},
	{'RS',	"Warner Bros. Interactive Entertainment Inc."},
	{'RT',	"RTL Games"},
	{'RW',	"RealNetworks"},
	{'S5',	"Southpeak Interactive"},
	{'SP',	"Blade Interactive Studios"},
	{'SV',	"SevenGames"},
	{'SZ',	"Storm City"},
	{'TK',	"Tasuke / Works"},
	{'TV',	"Tivola"},
	{'UG',	"Metro 3D / Data Design"},
	{'VN',	"Valcon Games"},
	{'VP',	"Virgin Play"},
	{'VZ',	"Little Orbit"},
	{'WR',	"Warner Bros. Interactive Entertainment Inc."},
	{'XJ',	"Xseed Games"},
	{'XS',	"Aksys Games"},
	{'XT',	"Fun Box Media"},
	{'YF',	"O2 Games"},
	{'YM',	"Bergsala Lightweight"},
	{'YT',	"Valcon Games"},
	{'Z4',	"Ntreev Soft"},
	{'ZA',	"WBA Interactive"},
	{'ZH',	"Internal Engine"},
	{'ZS',	"Zinkia"},
	{'ZW',	"Judo Baby"},
	{'ZX',	"Topware Interactive"},

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
int RP_C_API NintendoPublishersPrivate::compar(const void *a, const void *b)
{
	uint16_t code1 = static_cast<const ThirdPartyEntry*>(a)->code;
	uint16_t code2 = static_cast<const ThirdPartyEntry*>(b)->code;
	if (code1 < code2) return -1;
	if (code1 > code2) return 1;
	return 0;
}

/**
 * Nintendo third-party publisher list.
 * This list is valid for Famicom Disk System only.
 *
 * References:
 * - https://wiki.nesdev.com/w/index.php/Family_Computer_Disk_System#Manufacturer_codes
 */
const NintendoPublishersPrivate::ThirdPartyEntry_fds NintendoPublishersPrivate::thirdPartyList_fds[] = {
	{0x00,	"<unlicensed>",			"<非公認>"},
	{0x01,	"Nintendo",			"任天堂"},
	{0x08,	"Capcom",			"カプコン"},
	{0x0A,	"Jaleco",			"ジャレコ"},
	{0x18,	"Hudson Soft",			"ハドソン"},
	{0x49,	"Irem",				"アイレム"},
	{0x4A,	"Gakken",			"学習研究社"},
	{0x8B,	"BulletProof Software (BPS)",	"BPS"},
	{0x99,	"Pack-In-Video",		"パックインビデオ"},
	{0x9B,	"Tecmo",			"テクモ"},
	{0x9C,	"Imagineer",			"イマジニア"},
	{0xA2,	"Scorpion Soft",		"スコーピオンソフト"},
	{0xA4,	"Konami",			"コナミ"},
	{0xA6,	"Kawada Co., Ltd.",		"河田"},
	{0xA7,	"Takara",			"タカラ"},
	{0xA8,	"Royal Industries",		"ロイヤル工業"},
	{0xAC,	"Toei Animation",		"東映動画"},
	{0xAF,	"Namco",			"ナムコ"},
	{0xB1,	"ASCII Corporation",		"アスキー"},
	{0xB2,	"Bandai",			"バンダイ"},
	{0xB3,	"Soft Pro Inc.",		"ソフトプロ"},
	{0xB6,	"HAL Laboratory",		"HAL研究所"},
	{0xBB,	"Sunsoft",			"サンソフト"},
	{0xBC,	"Toshiba EMI",			"東芝EMI"},
	{0xC0,	"Taito",			"タイトー"},
	{0xC1,	"Sunsoft / Ask Co., Ltd.",	"サンソフト アスク講談社"},
	{0xC2,	"Kemco",			"ケムコ"},
	{0xC3,	"Square",			"スクウェア"},
	{0xC4,	"Tokuma Shoten",		"徳間書店"},
	{0xC5,	"Data East",			"データイースト"},
	{0xC6,	"Tonkin House / Tokyo Shoseki",	"トンキンハウス"},
	{0xC7,	"East Cube",			"イーストキューブ"},
	{0xCA,	"Konami / Ultra / Palcom",	"コナミ"},
	{0xCB,	"NTVIC / VAP",			"バップ"},
	{0xCC,	"Use Co., Ltd.",		"ユース"},
	{0xCE,	"Pony Canyon / FCI",		"ポニーキャニオン"},
	{0xD1,	"Sofel",			"ソフエル"},
	{0xD2,	"Bothtec, Inc.",		"ボーステック"},
	{0xDB,	"Hiro Co., Ltd.",		"ヒロ"},
	{0xE7,	"Athena",			"アテナ"},
	{0xEB,	"Atlus",			"アトラス"},

	{0, nullptr, nullptr}
};

/**
 * Comparison function for bsearch().
 * For use with ThirdPartyEntry.
 *
 * @param a
 * @param b
 * @return
 */
int RP_C_API NintendoPublishersPrivate::compar_fds(const void *a, const void *b)
{
	uint8_t code1 = static_cast<const ThirdPartyEntry_fds*>(a)->code;
	uint8_t code2 = static_cast<const ThirdPartyEntry_fds*>(b)->code;
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
const char *NintendoPublishers::lookup(uint16_t code)
{
	// Do a binary search.
	const NintendoPublishersPrivate::ThirdPartyEntry key = {code, nullptr};
	const NintendoPublishersPrivate::ThirdPartyEntry *res =
		static_cast<const NintendoPublishersPrivate::ThirdPartyEntry*>(bsearch(&key,
			NintendoPublishersPrivate::thirdPartyList,
			ARRAY_SIZE(NintendoPublishersPrivate::thirdPartyList)-1,
			sizeof(NintendoPublishersPrivate::ThirdPartyEntry),
			NintendoPublishersPrivate::compar));
	return (res ? res->publisher : nullptr);
}

/**
 * Look up a company code.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *NintendoPublishers::lookup(const char *code)
{
	if (!code[0] || !code[1])
		return nullptr;
	uint16_t code16 = ((uint8_t)(code[0]) << 8) |
			   (uint8_t)(code[1]);
	return lookup(code16);
}

/**
 * Look up a company code.
 * This uses the *old* company code, present in
 * older Game Boy titles.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *NintendoPublishers::lookup_old(uint8_t code)
{
	static const uint8_t hex_lookup[16] = {
		'0','1','2','3','4','5','6','7',
		'8','9','A','B','C','D','E','F'
	};
	uint16_t code16 = (hex_lookup[code >> 4] << 8) |
			   hex_lookup[code & 0x0F];
	return lookup(code16);
}

/**
 * Look up a company code for FDS titles.
 * This uses the *old* company code format.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *NintendoPublishers::lookup_fds(uint8_t code)
{
	// Do a binary search.
	// TODO: Option to return the Japanese publisher.
	const NintendoPublishersPrivate::ThirdPartyEntry_fds key = {code, nullptr, nullptr};
	const NintendoPublishersPrivate::ThirdPartyEntry_fds *res =
		static_cast<const NintendoPublishersPrivate::ThirdPartyEntry_fds*>(bsearch(&key,
			NintendoPublishersPrivate::thirdPartyList_fds,
			ARRAY_SIZE(NintendoPublishersPrivate::thirdPartyList_fds)-1,
			sizeof(NintendoPublishersPrivate::ThirdPartyEntry_fds),
			NintendoPublishersPrivate::compar_fds));
	return (res ? res->publisher_en : nullptr);
}

}
