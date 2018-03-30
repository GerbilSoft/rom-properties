/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUData.hpp: Nintendo Wii U publisher data.                            *
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

#include "WiiUData.hpp"

// C includes.
#include <stdlib.h>

namespace LibRomData {

class WiiUDataPrivate {
	private:
		// Static class.
		WiiUDataPrivate();
		~WiiUDataPrivate();
		RP_DISABLE_COPY(WiiUDataPrivate)

	public:
		struct WiiUDiscPublisher {
			uint32_t id4;		// Packed ID4.
			uint32_t publisher;	// Packed publisher ID.
		};

		/**
		 * Wii U retail disc publisher list. (region-independent)
		 * These games have the same publisher in all regions.
		 * ID4 region byte is 'x'. (Not '\0' due to MSVC issues.)
		 *
		 * Reference: http://www.gametdb.com/WiiU/List
		 */
		static const WiiUDiscPublisher disc_publishers_noregion[];

		/**
		 * Wii U retail disc publisher list. (region-specific)
		 * These games have different publishers in different regions.
		 * ID4 region byte is the original region.
		 *
		 * Reference: http://www.gametdb.com/WiiU/List
		 */
		static const WiiUDiscPublisher disc_publishers_region[];

		/**
		 * Comparison function for bsearch().
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API compar(const void *a, const void *b);
};

/** WiiUDataPrivate **/

/**
 * Wii U retail disc publisher list. (region-independent)
 * These games have the same publisher in all regions.
 * ID4 region byte is 'x'. (Not '\0' due to MSVC issues.)
 *
 * Reference: http://www.gametdb.com/WiiU/List
 */
const WiiUDataPrivate::WiiUDiscPublisher WiiUDataPrivate::disc_publishers_noregion[] = {
	{'AAFx', '0001'},	// Bayonetta
	{'AALx', '0001'},	// Animal Crossing: amiibo Festival
	{'ABAx', '0001'},	// Mario Party 10
	{'ABBx', '00VZ'},	// Barbie: Dreamhouse Party
	{'ABFx', '0052'},	// Rapala Pro Bass Fishing
	{'ABHx', '0008'},	// Resident Evil: Revelations
	{'ABJx', '0001'},	// Mario & Sonic at the Rio 2016 Olympic Games
	{'ABLx', '00YF'},	// Baila Latino
	{'ABPx', '00DV'},	// Brunswick Pro Bowling
	{'ABTx', '00WR'},	// Batman Arkham City: Armored Edition
	{'ABWx', '00AF'},	// Kamen Rider: Battride War II
	{'AC2x', '00VZ'},	// Monster High: 13 Wishes
	{'AC3x', '0001'},	// Pikmin 3
	{'ACAx', '0052'},	// Cabela's Dangerous Hunts 2013
	{'ACCx', '00NK'},	// Cocoto Magic Circus 2
	{'ACEx', '0052'},	// Cabela's Big Game Hunter: Pro Hunts
	{'ACMx', '0001'},	// The Wonderful 101
	{'ACPx', '0052'},	// Call of Duty: Ghosts
	{'AD2x', '0078'},	// Darksiders II
	{'ADCx', '00WR'},	// Scribblenauts Unmasked: a DC Comics Adventure
	{'ADNx', '0001'},	// Devil's Third
	{'ADPx', '0041'},	// Just Dance Disney Party 2
	{'ADQx', '00GD'},	// Dragon Quest X: Mezameshi Itsutsu no Shuzoku
	{'ADRx', '004Q'},	// Disney Infinity: Marvel Super Heroes - 2.0 Edition
	{'ADXx', '00GD'},	// Deus Ex: Human Revolution - Director's Cut
	{'AECx', '0052'},	// Call of Duty: Black Ops II
	{'AEMx', '004Q'},	// Disney Epic Mickey 2: The Power of Two
	{'AF3x', '0069'},	// FIFA Soccer 13
	{'AF6x', '0052'},	// Fast and Furious: Showdown
	{'AF9x', '0036'},	// F1 Race Stars: Powered Up Edition
	{'AFBx', '00GT'},	// Funky Barn
	{'AFDx', '0001'},	// NES Remix Pack
	{'AFMx', '00YF'},	// Fit Music for Wii U
	{'AFRx', '00VZ'},	// Disney Planes: Fire & Rescue
	{'AFXx', '0001'},	// Star Fox Zero
	{'AG5x', '0008'},	// Monster Hunter Frontier G5: Premium Package
	{'AG6x', '0008'},	// Monster Hunter Frontier G6: Premium Package
	{'AG9x', '00C8'},	// Sangokushi 12
	{'AGMx', '0001'},	// Splatoon
	{'AGNx', '0052'},	// Guitar Hero Live
	{'AGPx', '00WR'},	// Game Party Champions
	{'AGRx', '0052'},	// Angry Birds Star Wars
	{'AGZx', '00ME'},	// Ginsei Shogi: Kyoutendo Toufuu Raijin
	{'AH5x', '00NK'},	// Hunter's Trophy 2 - Europa
	{'AH9x', '00C8'},	// Fist of the North Star - Ken's Rage 2 (Shin Hokuto Musou)
	{'AHBx', '00C8'},	// Warriors Orochi 3 Hyper
	{'AHDx', '0008'},	// Monster Hunter 3 Ultimate
	{'AHFx', '00WY'},	// Shantae Half-Genie Hero
	{'AHKx', '00YM'},	// Hello Kitty Kruisers
	{'AHWx', '00WR'},	// Hot Wheels: World's Best Driver
	{'AJ5x', '0041'},	// Just Dance 2014
	{'AJ6x', '0041'},	// Just Dance 2016
	{'AJDx', '0041'},	// Just Dance 4
	{'AJKx', '0041'},	// Just Dance Kids 2014
	{'AJPx', '0078'},	// Jeopardy!
	{'AJSx', '00WR'},	// Injustice: Gods Among Us
	{'AJTx', '00XT'},	// Jett Tailfin
	{'AKBx', '0001'},	// Captain Toad: Treasure Tracker
	{'AKNx', '00AF'},	// Tekken Tag Tournament 2
	{'AL5x', '0001'},	// Project Zero: Maiden of Black Water
	{'ALAx', '00WR'},	// The LEGO Movie Videogame
	{'ALBx', '00WR'},	// LEGO Batman 2: DC Super Heroes
	{'ALCx', '0001'},	// Nintendo Land
	{'ALHx', '00WR'},	// LEGO The Hobbit
	{'ALJx', '00WR'},	// LEGO Jurassic World
	{'ALMx', '00WR'},	// LEGO Marvel Super Heroes
	{'ALRx', '00WR'},	// LEGO Marvel's Avengers
	{'ALVx', '00YF'},	// Luv Me Buddies Wonderland
	{'ALZx', '0001'},	// The Legend of Zelda: Breath of the Wild
	{'AM7x', '0008'},	// Monster Hunter Frontier G7 Premium Package
	{'AM8x', '0008'},	// Monster Hunter Frontier G8 Premium Package
	{'AM9x', '0052'},	// The Amazing Spider-Man 2
	{'AMAx', '0001'},	// Super Mario Maker
	{'AMDx', '0069'},	// Madden NFL 13
	{'AMEx', '0069'},	// Mass Effect 3: Special Edition
	{'AMFx', '0008'},	// Monster Hunter: Frontier G
	{'AMKx', '0001'},	// Mario Kart 8
	{'AMQx', '00KM'},	// Mighty No. 9
	{'AMVx', '0041'},	// Marvel Avengers: Battle for Earth
	{'AMZx', '0052'},	// The Amazing Spider-Man Ultimate Edition
	{'ANBx', '0054'},	// NBA 2K13
	{'ANGx', '0001'},	// Ninja Gaiden 3: Razor's Edge
	{'ANRx', '0052'},	// Angry Birds Trilogy
	{'ANSx', '0069'},	// Need for Speed: Most Wanted U
	{'ANXx', '0001'},	// Wii Party U
	{'ANYx', '00GD'},	// Dragon Quest X: Nemureru Yuusha to Michibiki no Meiyuu Online
	{'APAx', '004Q'},	// Disney Planes
	{'APCx', '00AF'},	// Pac-Man and the Ghostly Adventures
	{'APGx', '00VZ'},	// Penguins of Madagascar
	{'APKx', '0001'},	// Pokkén Tournament
	{'APLx', '0001'},	// LEGO City Undercover
	{'APPx', '0008'},	// Monster Hunter Frontier GG Premium Package
	{'APTx', '008P'},	// Puyo Puyo Tetris
	{'APUx', '00C8'},	// Sangokushi 12 with Power-Up Kit
	{'APWx', '00AF'},	// Kamen Rider: Battride War II (Premium TV & Movie Sound Edition)
	{'APZx', '00WR'},	// LEGO Dimensions
	{'AQUx', '0001'},	// Bayonetta 2 (TODO: Standalone or bundled?)
	{'ARBx', '0041'},	// Rabbids Land
	{'ARDx', '0001'},	// Super Mario 3D World
	{'ARKx', '0001'},	// Donkey Kong Country: Tropical Freeze
	{'ARMx', '0041'},	// Rayman Legends
	{'ARPx', '0001'},	// New Super Mario Bros. U
	{'ARSx', '0001'},	// New Super Luigi U
	{'ARYx', '008P'},	// Yakuza 1 & 2 HD
	{'AS2x', '008P'},	// Sonic & All-Stars Racing Transformed
	{'AS5x', '0052'},	// SpongeBob SquarePants: Plankton's Robotic Revenge
	{'AS7x', '0052'},	// Skylanders: Spyro's Adventure
	{'AS9x', '0041'},	// Tom Clancy's Splinter Cell Blacklist
	{'ASAx', '0001'},	// Game & Wario
	{'ASBx', '0041'},	// Assassin's Creed IV: Black Flag
	{'ASEx', '0001'},	// Tokyo Mirage Sessions #FE
	{'ASFx', '0052'},	// Skylanders: Swap Force
	{'ASLx', '0052'},	// Skylanders: Giants
	{'ASNx', '008P'},	// Sonic Lost World
	{'ASPx', '0041'},	// ESPN Sports Connection
	{'ASSE', '0041'},	// Assassin's Creed III
	{'ASTx', '0001'},	// Wii Fit U
	{'ASUx', '0041'},	// The Smurfs 2
	{'ASVx', '0052'},	// 007 Legends
	{'ASWx', '0001'},	// SiNG Party
	{'AT5x', '00AF'},	// Taiko no Tatsujin: Wii U Version!
	{'AT7x', '006D'},	// Tumblestone
	{'ATDx', '00VZ'},	// How to Train Your Dragon 2
	{'ATKx', '00AF'},	// Tank! Tank! Tank!
	{'ATRx', '0052'},	// Transformers: Prime
	{'ATWx', '0001'},	// New Super Mario Bros. U + New Super Luigi U
	// FIXME: Minecraft: Super Mario Mash-Up is AUMxD2.
	{'AUMx', '00DU'},	// Minecraft: Wii U Edition
	{'AUNx', '00AF'},	// One Piece: Unlimited World Red
	{'AURx', '0001'},	// Mario & Sonic at the Sochi 2014 Olympic Winter Games
	{'AVAJ', '00HF'},	// Youkai Watch Dance: Just Dance Special Version
	{'AVCx', '0052'},	// The Voice: I Want You
	{'AVXx', '0001'},	// Mario Tennis: Ultra Smash
	{'AV3x', '0052'},	// Wipeout 3
	{'AV4x', '0052'},	// Wipeout: Create & Crash
	{'AWCx', '0041'},	// Watch Dogs
	{'AWDx', '0052'},	// The Walking Dead: Survival Instinct
	{'AWFx', '0078'},	// Wheel of Fortune
	{'AWSx', '0001'},	// Wii Sports Club
	{'AX5x', '0001'},	// Xenoblade Chronicles X
	{'AXFx', '0001'},	// Super Smash Bros. for Wii U
	{'AXYx', '0001'},	// Kirby and the Rainbow Curse
	{'AYCx', '0001'},	// Yoshi's Woolly World
	{'AYEx', '0052'},	// Transformers: Rise of the Dark Spark
	{'AYSx', '0041'},	// Your Shape: Fitness Evolved 2013
	{'AZAx', '0001'},	// The Legend of Zelda: Twilight Princess HD
	{'AZBx', '005G'},	// Zumba Fitness World Party
	{'AZEx', '00WR'},	// Batman: Arkham Origins
	{'AZUx', '0041'},	// ZombiU
	{'BA4x', '00WR'},	// Cars 3: Driven to Win
	{'BAKx', '00TL'},	// Minecraft: Story Mode - The Complete Adventure
	{'BCZx', '0001'},	// The Legend of Zelda: The Wind Waker HD
	{'BD3x', '004Q'},	// Disney Infinity 3.0
	{'BDLx', '00GD'},	// Dragon Quest X Inishie no Ryu no Denshou Online
	{'BDQx', '00GD'},	// Dragon Quest X All in One Package
	{'BEDx', '0078'},	// Darksiders - Warmastered Edition
	{'BENx', '008X'},	// Runbow Deluxe
	{'BFNx', '00VZ'},	// Adventure Time: Finn & Jake Investigations
	{'BH9x', '0008'},	// Monster Hunter Frontier G9 Premium Package
	{'BJ7x', '0041'},	// Just Dance 2017
	{'BJDx', '0041'},	// Just Dance 2015
	{'BK7x', '0052'},	// Skylanders: Trap Team
	{'BKFx', '00VZ'},	// Kung Fu Pands: Showdown of Legendary Legends
	{'BKSx', '007W'},	// Demonic Karma Summoner
	{'BL6x', '0052'},	// Skylanders Imaginators
	{'BLGx', '00WR'},	// LEGO Star Wars: The Force Awakens
	{'BLKx', '006V'},	// Legend of Kay Anniversary
	{'BLTx', '00AF'},	// Gotouchi Tetsudou: Gotouchi Chara to Nihon Zenkoku no Tabi
	{'BMSx', '00VZ'},	// Monster High: New Ghoul in School
	{'BPCx', '0001'},	// Bayonetta 2 (TODO: Standalone or bundled?)
	{'BPEx', '0052'},	// The Peanuts Movie: Snoopy's Grand Adventure
	{'BPMx', '00AF'},	// Pac-Man and the Ghostly Adventures 2
	{'BR5x', '00JX'},	// FAST Racing NEO
	{'BRQx', '00VZ'},	// Barbie and Her Sisters: Puppy Rescue
	{'BRSx', '00AF'},	// Kamen Rider SummonRide
	{'BS5x', '0052'},	// Skylanders: SuperChargers
	{'BSFx', '00AF'},	// F. Fujio Characters Daishuugou! SF Dotabata Party!
	{'BSSx', '008P'},	// Sonic Boom: Rise of Lyric
	{'BT3x', '00AF'},	// Taiko no Tatsujin: Atsumete Tomodachi Daisakusen!
	{'BT9x', '00AF'},	// Taiko no Tatsujin: Tokumori! (NTSC-J)
	{'BTMx', '00WR'},	// LEGO Batman 3: Beyond Gotham
	{'BTPx', '00GD'},	// Dragon Quest X (All in One Package)
	{'BUTx', '006V'},	// The Book of Unwritten Tales 2
	{'BWFx', '0001'},	// Star Fox Guard
	{'BXAx', '0001'},	// Art Academy: Atelier
	{'CNFx', '0001'},	// Paper Mario: Color Splash
	// NOTE: These publisher IDs do NOT match the NintendoPublishers list!
	// (Also '3A' for Teslagrad.)
	{'SNCx', '0067'},	// (Event Preview) Freedom Planet
	{'SNGx', '000P'},	// (Event Preview) Runbow
	{'SNHx', '009V'},	// (Event Preview) Soul Axiom
	{'SNJx', '006J'},	// (Event Preview) Typoman
	// Disc versions of eShop titles.
	{'WAFx', '0001'},	// Mairo vs. Donkey Kong: Tipping Stars
	{'WAHx', '0001'},	// Wii Karaoke U (Trial Disc)
	{'WNCx', '0001'},	// Pokémon Rumble U: Special Edition
	{'WDKx', '0008'},	// DuckTales: Remastered
	{'WGDx', '003A'},	// Teslagrad
	{'WKNx', '00AY'},	// Shovel Knight

	{0, 0}
};

/**
 * Wii U retail disc publisher list. (region-specific)
 * These games have different publishers in different regions.
 * ID4 region byte is the original region.
 *
 * Reference: http://www.gametdb.com/WiiU/List
 */
const WiiUDataPrivate::WiiUDiscPublisher WiiUDataPrivate::disc_publishers_region[] = {
	{'ABEE', '00G9'},	// Ben 10: Omniverse (NTSC-U)
	{'ABEP', '00AF'},	// Ben 10: Omniverse (PAL)
	{'ABVE', '00G9'},	// Ben 10: Omniverse 2 (NTSC-U)
	{'ABVP', '00AF'},	// Ben 10: Omniverse 2 (PAL)
	{'ACRE', '00G9'},	// The Croods: Prehistoric Party! (NTSC-U)
	{'ACRP', '00AF'},	// The Croods: Prehistoric Party! (PAL)
	{'ADRE', '004Q'},	// Disney Infinity (NTSC-U)
	{'ADRJ', '00AF'},	// Disney Infinity (NTSC-J)
	{'ADRP', '004Q'},	// Disney Infinity (PAL)
	{'ADRZ', '004Q'},	// Disney Infinity (PAL)
	{'ADVE', '00G9'},	// Adventure Time: Explore the Dungeon Because I DON'T KNOW! (NTSC-U)
	{'ADVP', '00AF'},	// Adventure Time: Explore the Dungeon Because I DON'T KNOW! (PAL)
	{'AFPE', '00G9'},	// Family Party: 30 Great Games Obstacle Arcade (NTSC-U)
	{'AFPJ', '00G9'},	// Simple Wii U Series Vol. 1: The Family Party (NTSC-J)
	{'AFPP', '00AF'},	// Family Party: 30 Great Games Obstacle Arcade (PAL)
	{'AJ8E', '0099'},	// SteamWorld Collection (NTSC-U)
	{'AJ8P', '00VW'},	// SteamWorld Collection (PAL)
	{'APFE', '005G'},	// Phineas and Ferb: Quest for Cool Stuff (NTSC-U)
	{'APFP', '00GT'},	// Phineas and Ferb: Quest for Cool Stuff (PAL)
	{'ARGE', '00G9'},	// Rise of the Guardians (NTSC-U)
	{'ARGP', '00AF'},	// Rise of the Guardians (PAL)
	{'AS8E', '00GT'},	// Sniper Elite V2 (NTSC-U)
	{'AS8J', '0041'},	// Sniper Elite V2 (NTSC-J)
	{'AS8P', '00GT'},	// Sniper Elite V2 (PAL)
	{'ASCE', '00WR'},	// Scribblenauts Unlimited (NTSC-U)
	{'ASCP', '0001'},	// Scribblenauts Unlimited (PAL)
	{'ATBE', '00G9'},	// Turbo: Super Stunt Squad (NTSC-U)
	{'ATBP', '00AF'},	// Turbo: Super Stunt Squad (PAL)
	{'BRDE', '00NS'},	// Rodea the Sky Soldier (NTSC-U)
	{'BRDJ', '008J'},	// Rodea the Sky Soldier (NTSC-J)
	{'BRDP', '00NS'},	// Rodea the Sky Soldier (PAL)
	{'BTXE', '00GT'},	// Terraria (NTSC-U)
	{'BTXJ', '00EL'},	// Terraria (NTSC-J)
	{'BTXP', '00GT'},	// Terraria (PAL)
	{'BWPE', '0001'},	// Hyrule Warriors (NTSC-U)
	{'BWPJ', '00C8'},	// Zelda Musou (NTSC-J)
	{'BWPP', '0001'},	// Hyrule Warriors (PAL)

	{0, 0}
};

/**
 * Comparison function for bsearch().
 * @param a
 * @param b
 * @return
 */
int RP_C_API WiiUDataPrivate::compar(const void *a, const void *b)
{
	unsigned int id4_1 = static_cast<const WiiUDiscPublisher*>(a)->id4;
	unsigned int id4_2 = static_cast<const WiiUDiscPublisher*>(b)->id4;
	if (id4_1 < id4_2) return -1;
	if (id4_1 > id4_2) return 1;
	return 0;
}

/** WiiUData **/

/**
 * Look up a Wii U retail disc publisher.
 *
 * NOTE: Wii U uses 4-character publisher IDs.
 * If the first two characters are "00", then it's
 * equivalent to a 2-character publisher ID.
 *
 * @param Wii U retail disc ID4.
 * @return Packed publisher ID, or 0 if not found.
 */
uint32_t WiiUData::lookup_disc_publisher(const char *id4)
{
	// Check the region-independent list first.
	WiiUDataPrivate::WiiUDiscPublisher key;
	key.id4 = ((uint8_t)id4[0] << 24) |
		  ((uint8_t)id4[1] << 16) |
		  ((uint8_t)id4[2] << 8) | 'x';
	key.publisher = 0;

	// Do a binary search.
	const WiiUDataPrivate::WiiUDiscPublisher *res =
		static_cast<const WiiUDataPrivate::WiiUDiscPublisher*>(bsearch(&key,
			WiiUDataPrivate::disc_publishers_noregion,
			ARRAY_SIZE(WiiUDataPrivate::disc_publishers_noregion)-1,
			sizeof(WiiUDataPrivate::WiiUDiscPublisher),
			WiiUDataPrivate::compar));
	if (res) {
		// Found a publisher in the region-independent list.
		return res->publisher;
	}

	// Check the region-specific list.
	key.id4 &= ~0xFF;
	key.id4 |= (uint8_t)id4[3];

	// Do a binary search.
	res = static_cast<const WiiUDataPrivate::WiiUDiscPublisher*>(bsearch(&key,
			WiiUDataPrivate::disc_publishers_region,
			ARRAY_SIZE(WiiUDataPrivate::disc_publishers_region)-1,
			sizeof(WiiUDataPrivate::WiiUDiscPublisher),
			WiiUDataPrivate::compar));

	return (res ? res->publisher : 0);
}

}
