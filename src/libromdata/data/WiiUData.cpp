/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUData.hpp: Nintendo Wii U data.                                      *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WiiUData.hpp"

// C++ STL classes
using std::array;

namespace LibRomData { namespace WiiUData {

struct WiiUDiscPublisher {
	uint32_t id4;		// Packed ID4.
	uint32_t publisher;	// Packed publisher ID.
};

/**
 * Wii U retail disc publisher list. (region-independent)
 * These games have the same publisher in all regions.
 * ID4 region byte is 'x'. (Not '\0' due to MSVC issues.)
 *
 * Reference: https://www.gametdb.com/WiiU/List
 */
static const array<WiiUDiscPublisher, 199> disc_publishers_noregion = {{
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
	{'AE9x', '0026'},	// Shmup Collection
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
	{'AQUx', '0001'},	// Bayonetta 2
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
	{'BPCx', '0001'},	// Bayonetta 2 (Special Edition)
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
	{'HEHx', '0026'},	// Finding Teddy II - Definitive Edition
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
	{'WGSx', '00CX'},	// Giana Sisters: Twisted Dreams - Director's Cut
	{'WKNx', '00AY'},	// Shovel Knight
	{'WTBx', '00EY'},	// Shakedown: Hawaii
}};

/**
 * Wii U retail disc publisher list. (region-specific)
 * These games have different publishers in different regions.
 * ID4 region byte is the original region.
 *
 * Reference: https://www.gametdb.com/WiiU/List
 */
static const array<WiiUDiscPublisher, 37> disc_publishers_region = {{
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
}};

/** Public functions **/

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
uint32_t lookup_disc_publisher(const char *id4)
{
	// Check the region-independent list first.
	uint32_t id4_u32 = (static_cast<uint8_t>(id4[0]) << 24) |
	                   (static_cast<uint8_t>(id4[1]) << 16) |
	                   (static_cast<uint8_t>(id4[2]) << 8) | 'x';

	// Do a binary search.
	auto pPubNoRegion = std::lower_bound(disc_publishers_noregion.cbegin(), disc_publishers_noregion.cend(), id4_u32,
		[](const WiiUDiscPublisher &pub, const uint32_t id4_u32) noexcept -> bool {
			return (pub.id4 < id4_u32);
		});
	if (pPubNoRegion != disc_publishers_noregion.cend() && pPubNoRegion->id4 == id4_u32) {
		// Found a publisher in the region-independent list.
		return pPubNoRegion->publisher;
	}

	// Check the region-specific list.
	id4_u32 &= ~0xFF;
	id4_u32 |= static_cast<uint8_t>(id4[3]);

	// Do a binary search.
	auto pPubRegion = std::lower_bound(disc_publishers_region.cbegin(), disc_publishers_region.cend(), id4_u32,
		[](const WiiUDiscPublisher &pub, uint32_t id4_u32) noexcept -> bool {
			return (pub.id4 < id4_u32);
		});
	if (pPubRegion != disc_publishers_region.cend() && pPubRegion->id4 == id4_u32) {
		// Found a publisher in the region-dependent list.
		return pPubRegion->publisher;
	}

	// Not found.
	return 0;
}

struct WiiUApplicationType {
	uint32_t app_type;
	const char *desc;
};

/**
 * Wii U application types.
 * TODO: Convert into a string table?
 *
 * Reference: https://github.com/devkitPro/wut/blob/master/include/coreinit/mcp.h
 */
static const array<WiiUApplicationType, 45> wiiu_application_types = {{
	{0x0000001D, "Compat User"},
	{0x0800001B, "Game Update"},
	{0x0800000E, "Game DLC"},
	{0x10000009, "boot1"},
	{0x1000000A, "IOSU"},
	{0x10000011, "Compat System"},
	{0x10000012, "Bluetooth Firmware"},
	{0x10000013, "DRH Firmware"},
	{0x10000014, "DRC Firmware"},
	{0x10000015, "System Version"},
	{0x1000001A, "DRC Language"},
	{0x10000024, "In-Disc Patch"},
	{0x10000025, "OS Patch"},
	{0x1800000F, "Data System"},
	{0x18000010, "Exceptions Data"},
	{0x1800001C, "Shared RO Data"},
	{0x1800001E, "Cert Store"},
	{0x18000023, "Patch Map Data"},
	{0x18000029, "WagonU Migration List"},
	{0x18000030, "Caffeine Title List"},
	{0x18000031, "MCP Title List"},
	{0x20000007, "Update User"},
	{0x30000008, "Update System"},
	{0x80000000, "Game"},
	{0x8000001F, "Game(Inv)"},
	{0x8000002E, "Wii Game"},
	{0x90000001, "System Menu"},
	{0x9000000B, "System Updater"},
	{0x90000020, "System Applet"},
	{0x90000021, "Account Applet"},
	{0x90000022, "System Settings"},
	{0x9000002F, "Eco Process"},
	{0xD0000002, "Vino"},
	{0xD0000003, "eManual"},
	{0xD0000004, "HOME Button Menu"},
	{0xD0000005, "Error Display"},
	{0xD0000006, "Browser"},
	{0xD000000D, "Mini Miiverse"},
	{0xD0000016, "Miiverse"},
	{0xD0000017, "eShop"},
	{0xD0000018, "Friend List"},
	{0xD0000019, "Download Management"},
	{0xD000002B, "TestOverlay"},
	{0xD000002C, "eShop Overlay"},
	{0xD0000033, "amiibo Settings"},
}};

/**
 * Look up a Wii U application type.
 * @param app_type Application type ID
 * @return Application type string, or nullptr if unknown.
 */
const char *lookup_application_type(uint32_t app_type)
{
	auto p_app_type = std::lower_bound(wiiu_application_types.cbegin(), wiiu_application_types.cend(), app_type,
		[](const WiiUApplicationType &app_type_info, const uint32_t app_type) noexcept -> bool {
			return (app_type_info.app_type < app_type);
		});
	if (p_app_type != wiiu_application_types.cend() && p_app_type->app_type == app_type) {
		// Found a matching application type.
		// TODO: Localize this?
		return p_app_type->desc;
	}

	// Not found.
	return nullptr;
}

} }
