/** NES Mappers (generated from NESMappers_data.txt) **/

#include <stdint.h>

static const char NESMappers_strtbl[] =
	"\x00" "NROM" "\x00" "Nintendo" "\x00" "SxROM (MMC1)" "\x00" "UxR"
	"OM" "\x00" "CNROM" "\x00" "TxROM (MMC3), HKROM (MMC6)" "\x00" "E"
	"xROM (MMC5)" "\x00" "Game Doctor Mode 1" "\x00" "Bung/FFE" "\x00"
	"AxROM" "\x00" "Game Doctor Mode 4 (GxROM)" "\x00" "PxROM, PEEORO"
	"M (MMC2)" "\x00" "FxROM (MMC4)" "\x00" "Color Dreams" "\x00" "MM"
	"C3 variant" "\x00" "FFE" "\x00" "NES-CPROM" "\x00" "SL-1632 (MMC"
	"3/VRC2 clone)" "\x00" "K-1029 (multicart)" "\x00" "FCG-x" "\x00" "B"
	"andai" "\x00" "FFE #17" "\x00" "SS 88006" "\x00" "Jaleco" "\x00" "N"
	"amco 129/163" "\x00" "Namco" "\x00" "Famicom Disk System" "\x00" "V"
	"RC4a, VRC4c" "\x00" "Konami" "\x00" "VRC2a" "\x00" "VRC4e, VRC4f"
	", VRC2b" "\x00" "VRC6a" "\x00" "VRC4b, VRC4d, VRC2c" "\x00" "VRC"
	"6b" "\x00" "VRC4 variant" "\x00" "Action 53" "\x00" "Homebrew" "\x00"
	"RET-CUFROM" "\x00" "Sealie Computing" "\x00" "UNROM 512" "\x00" "R"
	"etroUSB" "\x00" "NSF Music Compilation" "\x00" "Irem G-101" "\x00"
	"Irem" "\x00" "Taito TC0190" "\x00" "Taito" "\x00" "BNROM, NINA-0"
	"01" "\x00" "J.Y. Company ASIC (8 KiB WRAM)" "\x00" "J.Y. Company"
	"\x00" "TXC PCB 01-22000-400" "\x00" "TXC" "\x00" "MMC3 multicart"
	"\x00" "GNROM variant" "\x00" "Bit Corp." "\x00" "BNROM variant" "\x00"
	"NTDEC 2722 (FDS conversion)" "\x00" "NTDEC" "\x00" "Caltron 6-in"
	"-1" "\x00" "Caltron" "\x00" "FDS conversion" "\x00" "TONY-I, YS-"
	"612 (FDS conversion)" "\x00" "MMC3 multicart (GA23C)" "\x00" "Ru"
	"mble Station 15-in-1" "\x00" "Taito TC0690" "\x00" "PCB 761214 ("
	"FDS conversion)" "\x00" "N-32" "\x00" "Novel Diamond 9999999-in-"
	"1" "\x00" "BTL-MARIO1-MALEE2" "\x00" "KS202 (unlicensed SMB3 rep"
	"roduction)" "\x00" "Multicart" "\x00" "(C)NROM-based multicart" "\x00"
	"BMC-T3H53/BMC-D1038 multicart" "\x00" "Reset-based NROM-128 4-in"
	"-1 multicart" "\x00" "20-in-1 multicart" "\x00" "Super 700-in-1 "
	"multicart" "\x00" "Powerful 250-in-1 multicart" "\x00" "Tengen R"
	"AMBO-1" "\x00" "Tengen" "\x00" "Irem H3001" "\x00" "GxROM, MHROM"
	"\x00" "Sunsoft-3" "\x00" "Sunsoft" "\x00" "Sunsoft-4" "\x00" "Su"
	"nsoft FME-7" "\x00" "Family Trainer" "\x00" "Codemasters (UNROM "
	"clone)" "\x00" "Codemasters" "\x00" "Jaleco JF-17" "\x00" "VRC3" "\x00"
	"43-393/860908C (MMC3 clone)" "\x00" "Waixing" "\x00" "VRC1" "\x00"
	"NAMCOT-3446 (Namcot 108 variant)" "\x00" "Napoleon Senki" "\x00" "L"
	"enar" "\x00" "Holy Diver; Uchuusen - Cosmo Carrier" "\x00" "NINA"
	"-03, NINA-06" "\x00" "American Video Entertainment" "\x00" "Tait"
	"o X1-005" "\x00" "Super Gun" "\x00" "Taito X1-017 (incorrect PRG"
	" ROM bank ordering)" "\x00" "Cony/Yoko" "\x00" "PC-SMB2J" "\x00" "V"
	"RC7" "\x00" "Jaleco JF-13" "\x00" "CNROM variant" "\x00" "Namcot"
	" 118 variant" "\x00" "Sunsoft-2 (Sunsoft-3 board)" "\x00" "J.Y. "
	"Company (simple nametable control)" "\x00" "J.Y. Company (Super "
	"Fighter III)" "\x00" "Moero!! Pro" "\x00" "Sunsoft-2 (Sunsoft-3R"
	" board)" "\x00" "HVC-UN1ROM" "\x00" "NAMCOT-3425" "\x00" "Oeka K"
	"ids" "\x00" "Irem TAM-S1" "\x00" "CNROM (Vs. System)" "\x00" "MM"
	"C3 variant (hacked ROMs)" "\x00" "Jaleco JF-10 (misdump)" "\x00" "J"
	"aleceo" "\x00" "Doki Doki Panic (FDS conversion)" "\x00" "PEGASU"
	"S 5 IN 1" "\x00" "NES-EVENT (MMC1 variant) (Nintendo World Champ"
	"ionships 1990)" "\x00" "Super Mario Bros. 3 (bootleg)" "\x00" "M"
	"agic Dragon" "\x00" "Magicseries" "\x00" "Cheapocabra GTROM 512k"
	" flash board" "\x00" "Membler Industries" "\x00" "NINA-03/06 mul"
	"ticart" "\x00" "MMC3 clone (scrambled registers)" "\x00" "K" "\xc7"
	"\x8e" "sh" "\xc3\xa8" "ng SFC-02B/-03/-004 (MMC3 clone)" "\x00" "K"
	"\xc7\x8e" "sh" "\xc3\xa8" "ng" "\x00" "SOMARI-P (Huang-1/Huang-2"
	")" "\x00" "Gouder" "\x00" "TxSROM" "\x00" "TQROM" "\x00" "K" "\xc7"
	"\x8e" "sh" "\xc3\xa8" "ng A9711 and A9713 (MMC3 clone)" "\x00" "K"
	"\xc7\x8e" "sh" "\xc3\xa8" "ng H2288 (MMC3 clone)" "\x00" "Monty "
	"no Doki Doki Daisass" "\xc5\x8d" " (FDS conversion)" "\x00" "Whi"
	"rlwind Manu" "\x00" "TXC 05-00002-010 ASIC" "\x00" "Jovial Race" "\x00"
	"Sachen" "\x00" "T4A54A, WX-KB4K, BS-5652 (MMC3 clone)" "\x00" "S"
	"achen 3011" "\x00" "Sachen 8259D" "\x00" "Sachen 8259B" "\x00" "S"
	"achen 8259C" "\x00" "Jaleco JF-11, JF-14 (GNROM variant)" "\x00" "S"
	"achen 8259A" "\x00" "Kaiser KS202 (FDS conversions)" "\x00" "Kai"
	"ser" "\x00" "Copy-protected NROM" "\x00" "Death Race (Color Drea"
	"ms variant)" "\x00" "American Game Cartridges" "\x00" "Sidewinde"
	"r (CNROM clone)" "\x00" "Galactic Crusader (NINA-06 clone)" "\x00"
	"Sachen 3018" "\x00" "Sachen SA-008-A, Tengen 800008" "\x00" "Sac"
	"hen / Tengen" "\x00" "SA-0036 (CNROM clone)" "\x00" "Sachen SA-0"
	"15, SA-630" "\x00" "VRC1 (Vs. System)" "\x00" "Kaiser KS202 (FDS"
	" conversion)" "\x00" "Bandai FCG: LZ93D50 with SRAM" "\x00" "NAM"
	"COT-3453" "\x00" "MMC1A" "\x00" "DIS23C01" "\x00" "Daou Infosys" "\x00"
	"Datach Joint ROM System" "\x00" "Tengen 800037" "\x00" "Bandai L"
	"Z93D50 with 24C01" "\x00" "Nanjing" "\x00" "Waixing (unlicensed)"
	"\x00" "Fire Emblem (unlicensed) (MMC2+MMC3 hybrid)" "\x00" "Subo"
	"r (variant 1)" "\x00" "Subor" "\x00" "Subor (variant 2)" "\x00" "R"
	"acermate Challenge 2" "\x00" "Racermate, Inc." "\x00" "Yuxing" "\x00"
	"Kaiser KS-7058" "\x00" "Super Mega P-4040" "\x00" "Idea-Tek ET-x"
	"x" "\x00" "Idea-Tek" "\x00" "Waixing multicart (MMC3 clone)" "\x00"
	"H" "\xc3\xa9" "ngg" "\xc3\xa9" " Di" "\xc3\xa0" "nz" "\xc7\x90\x00"
	"Waixing / Nanjing / Jncota / Henge Dianzi / GameStar" "\x00" "Cr"
	"azy Climber (UNROM clone)" "\x00" "Nichibutsu" "\x00" "Seicross "
	"v2 (FCEUX hack)" "\x00" "MMC3 clone (scrambled registers) (same "
	"as 114)" "\x00" "Suikan Pipe (VRC4e clone)" "\x00" "Sunsoft-1" "\x00"
	"CNROM with weak copy protection" "\x00" "Study Box" "\x00" "Fuku"
	"take Shoten" "\x00" "K" "\xc7\x8e" "sh" "\xc3\xa8" "ng A98402 (M"
	"MC3 clone)" "\x00" "Bandai Karaoke Studio" "\x00" "Thunder Warri"
	"or (MMC3 clone)" "\x00" "Magic Kid GooGoo" "\x00" "MMC3 clone" "\x00"
	"NTDEC TC-112" "\x00" "Waixing FS303 (MMC3 clone)" "\x00" "Mario "
	"bootleg (MMC3 clone)" "\x00" "K" "\xc7\x8e" "sh" "\xc3\xa8" "ng "
	"(MMC3 clone)" "\x00" "T" "\xc5\xab" "nsh" "\xc3\xad" " Ti" "\xc4"
	"\x81" "nd" "\xc3\xac" " - S" "\xc4\x81" "ngu" "\xc3\xb3" " W" "\xc3"
	"\xa0" "izhu" "\xc3\xa0" "n" "\x00" "Waixing (clone of either Map"
	"per 004 or 176)" "\x00" "NROM-256 multicart" "\x00" "150-in-1 mu"
	"lticart" "\x00" "35-in-1 multicart" "\x00" "DxROM (Tengen MIMIC-"
	"1, Namcot 118)" "\x00" "Fudou Myouou Den" "\x00" "Street Fighter"
	" IV (unlicensed) (MMC3 clone)" "\x00" "J.Y. Company (MMC2/MMC4 c"
	"lone)" "\x00" "Namcot 175, 340" "\x00" "J.Y. Company (extended n"
	"ametable control)" "\x00" "BMC Super HiK 300-in-1" "\x00" "(C)NR"
	"OM-based multicart (same as 058)" "\x00" "Sugar Softec (MMC3 clo"
	"ne)" "\x00" "Sugar Softec" "\x00" "Magic Floor" "\x00" "K" "\xc7"
	"\x8e" "sh" "\xc3\xa8" "ng A9461 (MMC3 clone)" "\x00" "Summer Car"
	"nival '92 - Recca" "\x00" "Naxat Soft" "\x00" "NTDEC N625092" "\x00"
	"CTC-31 (VRC2 + 74xx)" "\x00" "Jncota KT-008" "\x00" "Jncota" "\x00"
	"Active Enterprises" "\x00" "BMC 31-IN-1" "\x00" "Codemasters Qua"
	"ttro" "\x00" "Maxi 15 multicart" "\x00" "Golden Game 150-in-1 mu"
	"lticart" "\x00" "Realtec 8155" "\x00" "Realtec" "\x00" "Teletubb"
	"ies 420-in-1 multicart" "\x00" "BNROM variant (similar to 034)" "\x00"
	"Unlicensed" "\x00" "Sachen SA-020A" "\x00" "F" "\xc4\x93" "ngsh" "\xc3"
	"\xa9" "nb" "\xc7\x8e" "ng: F" "\xc3\xba" "m" "\xc3\xb3" " S" "\xc4"
	"\x81" "n T" "\xc3\xa0" "iz" "\xc7\x90" " (C&E)" "\x00" "C&E" "\x00"
	"K" "\xc7\x8e" "sh" "\xc3\xa8" "ng SFC-02B/-03/-004 (MMC3 clone) "
	"(incorrect assignment; should be 115)" "\x00" "Nitra (MMC3 clone"
	")" "\x00" "Nitra" "\x00" "Waixing - Sangokushi" "\x00" "Dragon B"
	"all Z: Ky" "\xc5\x8d" "sh" "\xc5\xab" "! Saiya-jin (VRC4 clone)" "\x00"
	"Pikachu Y2K of crypted ROMs" "\x00" "110-in-1 multicart (same as"
	" 225)" "\x00" "OneBus Famiclone" "\x00" "UNIF PEC-586" "\x00" "U"
	"NIF 158B" "\x00" "UNIF F-15 (MMC3 multicart)" "\x00" "HP10xx/HP2"
	"0xx multicart" "\x00" "200-in-1 Elfland multicart" "\x00" "Stree"
	"t Heroes (MMC3 clone)" "\x00" "King of Fighters '97 (MMC3 clone)"
	"\x00" "Cony/Yoko Fighting Games" "\x00" "T-262 multicart" "\x00" "C"
	"ity Fighter IV" "\x00" "8-in-1 JY-119 multicart (MMC3 clone)" "\x00"
	"SMD132/SMD133 (MMC3 clone)" "\x00" "Multicart (MMC3 clone)" "\x00"
	"Game Prince RS-16" "\x00" "TXC 4-in-1 multicart (MGC-026)" "\x00"
	"Akumaj" "\xc5\x8d" " Special: Boku Dracula-kun (bootleg)" "\x00" "G"
	"remlins 2 (bootleg)" "\x00" "Cartridge Story multicart" "\x00" "R"
	"CM Group" "\x00" "J.Y. Company Super HiK 3/4/5-in-1 multicart" "\x00"
	"J.Y. Company multicart" "\x00" "Block Family 6-in-1/7-in-1 multi"
	"cart" "\x00" "Drip" "\x00" "A65AS multicart" "\x00" "Benshieng m"
	"ulticart" "\x00" "Benshieng" "\x00" "4-in-1 multicart (411120-C,"
	" 811120-C)" "\x00" "GKCX1 21-in-1 multicart" "\x00" "BMC-60311C" "\x00"
	"Asder 20-in-1 multicart" "\x00" "Asder" "\x00" "K" "\xc7\x8e" "s"
	"h" "\xc3\xa8" "ng 2-in-1 multicart (MK6)" "\x00" "Dragon Fighter"
	" (unlicensed)" "\x00" "NewStar 12-in-1/76-in-1 multicart" "\x00" "T"
	"4A54A, WX-KB4K, BS-5652 (MMC3 clone) (same as 134)" "\x00" "J.Y."
	" Company 13-in-1 multicart" "\x00" "FC Pocket RS-20 / dreamGEAR "
	"My Arcade Gamer V" "\x00" "TXC 01-22110-000 multicart" "\x00" "L"
	"ethal Weapon (unlicensed) (VRC4 clone)" "\x00" "TXC 6-in-1 multi"
	"cart (MGC-023)" "\x00" "Golden 190-in-1 multicart" "\x00" "GG1 m"
	"ulticart" "\x00" "Gyruss (FDS conversion)" "\x00" "Almana no Kis"
	"eki (FDS conversion)" "\x00" "Dracula II: Noroi no F" "\xc5\xab" "i"
	"n (FDS conversion)" "\x00" "Exciting Basket (FDS conversion)" "\x00"
	"Metroid (FDS conversion)" "\x00" "Batman (Sunsoft) (bootleg) (VR"
	"C2 clone)" "\x00" "Ai Senshi Nicol (FDS conversion)" "\x00" "Mon"
	"ty no Doki Doki Daisass" "\xc5\x8d" " (FDS conversion) (same as "
	"125)" "\x00" "Highway Star (bootleg)" "\x00" "Reset-based multic"
	"art (MMC3)" "\x00" "Y2K multicart" "\x00" "820732C- or 830134C- "
	"multicart" "\x00" "Super HiK 6-in-1 A-030 multicart" "\x00" "35-"
	"in-1 (K-3033) multicart" "\x00" "Farid's homebrew 8-in-1 SLROM m"
	"ulticart" "\x00" "Farid's homebrew 8-in-1 UNROM multicart" "\x00"
	"Super Mali Splash Bomb (bootleg)" "\x00" "Contra/Gryzor (bootleg"
	")" "\x00" "6-in-1 multicart" "\x00" "Test Ver. 1.01 Dlya Proverk"
	"i TV Pristavok test cartridge" "\x00" "Education Computer 2000" "\x00"
	"Sangokushi II: Ha" "\xc5\x8d" " no Tairiku (bootleg)" "\x00" "7-"
	"in-1 (NS03) multicart" "\x00" "Super 40-in-1 multicart" "\x00" "N"
	"ew Star Super 8-in-1 multicart" "\x00" "New Star" "\x00" "5/20-i"
	"n-1 1993 Copyright multicart" "\x00" "10-in-1 multicart" "\x00" "1"
	"1-in-1 multicart" "\x00" "12-in-1 Game Card multicart" "\x00" "1"
	"6-in-1, 200/300/600/1000-in-1 multicart" "\x00" "21-in-1 multica"
	"rt" "\x00" "Simple 4-in-1 multicart" "\x00" "COOLGIRL multicart "
	"(Homebrew)" "\x00" "Kuai Da Jin Ka Zhong Ji Tiao Zhan 3-in-1 mul"
	"ticart" "\x00" "New Star 6-in-1 Game Cartridge multicart" "\x00" "Z"
	"anac (FDS conversion)" "\x00" "Yume Koujou: Doki Doki Panic (FDS"
	" conversion)" "\x00" "830118C" "\x00" "1994 Super HIK 14-in-1 (G"
	"-136) multicart" "\x00" "Super 15-in-1 Game Card multicart" "\x00"
	"9-in-1 multicart" "\x00" "J.Y. Company / Techline" "\x00" "92 Su"
	"per Mario Family multicart" "\x00" "250-in-1 multicart" "\x00\xe9"
	"\xbb\x83\xe4\xbf\xa1\xe7\xb6\xad" " 3D-BLOCK" "\x00" "7-in-1 Roc"
	"kman (JY-208)" "\x00" "4-in-1 (4602) multicart" "\x00" "SB-5013 "
	"/ GCL8050 / 841242C multicart" "\x00" "31-in-1 (3150) multicart" "\x00"
	"YY841101C multicart (MMC3 clone)" "\x00" "830506C multicart (VRC"
	"4f clone)" "\x00" "JY830832C multicart" "\x00" "Asder PC-95 educ"
	"ational computer" "\x00" "GN-45 multicart (MMC3 clone)" "\x00" "7"
	"-in-1 multicart" "\x00" "Super Mario Bros. 2 (J) (FDS conversion"
	")" "\x00" "YUNG-08" "\x00" "N49C-300" "\x00" "F600" "\x00" "Span"
	"ish PEC-586 home computer cartridge" "\x00" "Dongda" "\x00" "Roc"
	"kman 1-6 (SFC-12) multicart" "\x00" "Super 4-in-1 (SFC-13) multi"
	"cart" "\x00" "Reset-based MMC1 multicart" "\x00" "135-in-1 (U)NR"
	"OM multicart" "\x00" "YY841155C multicart" "\x00" "1998 Super Ga"
	"me 8-in-1 (JY-111)" "\x00" "8-in-1 AxROM/UNROM multicart" "\x00" "3"
	"5-in-1 NROM multicart" "\x00" "970630C" "\x00" "KN-42" "\x00" "8"
	"30928C" "\x00" "YY840708C (MMC3 clone)" "\x00" "L1A16 (VRC4e clo"
	"ne)" "\x00" "NTDEC 2779" "\x00" "YY860729C" "\x00" "YY850735C / "
	"YY850817C" "\x00" "YY841145C / YY850835C" "\x00" "Caltron 9-in-1"
	" multicart" "\x00" "Realtec 8031" "\x00" "NC7000M (MMC3 clone)" "\x00"
	"Zh" "\xc5\x8d" "nggu" "\xc3\xb3" " D" "\xc3\xa0" "h" "\xc4\x93" "n"
	"g" "\x00" "M" "\xc4\x9b" "i Sh" "\xc3\xa0" "on" "\xc7\x9a" " M" "\xc3"
	"\xa8" "ng G" "\xc5\x8d" "ngch" "\xc7\x8e" "ng III" "\x00" "Subor"
	" Karaoke" "\x00" "Family Noraebang" "\x00" "Brilliant Com Cocoma"
	" Pack" "\x00" "EduBank" "\x00" "Kkachi-wa Nolae Chingu" "\x00" "S"
	"ubor multicart" "\x00" "UNL-EH8813A" "\x00" "2-in-1 Datach multi"
	"cart (VRC4e clone)" "\x00" "Korean Igo" "\x00" "F" "\xc5\xab" "u"
	"n Sh" "\xc5\x8d" "rinken (FDS conversion)" "\x00" "F" "\xc4\x93" "n"
	"gsh" "\xc3\xa9" "nb" "\xc7\x8e" "ng: F" "\xc3\xba" "m" "\xc3\xb3"
	" S" "\xc4\x81" "n T" "\xc3\xa0" "iz" "\xc7\x90" " (Jncota)" "\x00"
	"The Lord of King (Jaleco) (bootleg)" "\x00" "UNL-KS7021A (VRC2b "
	"clone)" "\x00" "Sangokushi: Ch" "\xc5\xab" "gen no Hasha (bootle"
	"g)" "\x00" "Fud" "\xc5\x8d" " My" "\xc5\x8d\xc5\x8d" " Den (boot"
	"leg) (VRC2b clone)" "\x00" "1995 New Series Super 2-in-1 multica"
	"rt" "\x00" "Datach Dragon Ball Z (bootleg) (VRC4e clone)" "\x00" "S"
	"uper Mario Bros. Pocker Mali (VRC4f clone)" "\x00" "Sachen 3014" "\x00"
	"2-in-1 Sudoku/Gomoku (NJ064) (MMC3 clone)" "\x00" "Nazo no Muras"
	"amej" "\xc5\x8d" " (FDS conversion)" "\x00" "Waixing FS303 (MMC3"
	" clone) (same as 195)" "\x00" "60-1064-16L" "\x00" "Kid Icarus ("
	"FDS conversion)" "\x00" "Master Fighter VI' hack (variant of 359"
	")" "\x00" "LittleCom 160-in-1 multicart" "\x00" "World Hero hack"
	" (VRC4 clone)" "\x00" "5-in-1 (CH-501) multicart (MMC1 clone)" "\x00"
	"Waixing FS306" "\x00" "Konami QTa adapter (VRC5)" "\x00" "CTC-15"
	"\x00" "Co Tung Co." "\x00" "Jncota RPG re-release (variant of 17"
	"8)" "\x00" "Taito X1-017 (correct PRG ROM bank ordering)" "\x00";

typedef struct _NESMapperEntry {
	uint16_t name_idx;
	uint16_t mfr_idx;
	NESMirroring mirroring;
} NESMapperEntry;

static const NESMapperEntry NESMappers_offtbl[] = {
	/* Mapper 000 */
	{1, 6, NESMirroring::Header},
	{15, 6, NESMirroring::MapperHVAB},
	{28, 6, NESMirroring::Header},
	{34, 6, NESMirroring::Header},
	{40, 6, NESMirroring::MapperHV},
	{67, 6, NESMirroring::MapperMMC5},
	{80, 99, NESMirroring::MapperHVAB},
	{108, 6, NESMirroring::MapperAB},
	{114, 99, NESMirroring::MapperHVAB},
	{141, 6, NESMirroring::MapperHV},

	/* Mapper 010 */
	{163, 6, NESMirroring::MapperHV},
	{176, 176, NESMirroring::Header},
	{189, 202, NESMirroring::MapperHV},
	{206, 6, NESMirroring::Header},
	{216, 6, NESMirroring::MapperHVAB},
	{242, 0, NESMirroring::MapperHV},
	{261, 267, NESMirroring::MapperHVAB},
	{274, 202, NESMirroring::MapperHVAB},
	{282, 291, NESMirroring::MapperHVAB},
	{298, 312, NESMirroring::MapperNamco163},

	/* Mapper 020 */
	{318, 6, NESMirroring::MapperHV},
	{338, 351, NESMirroring::MapperHVAB},
	{358, 351, NESMirroring::MapperHVAB},
	{364, 351, NESMirroring::MapperHVAB},
	{384, 351, NESMirroring::MapperVRC6},
	{390, 351, NESMirroring::MapperHVAB},
	{410, 351, NESMirroring::MapperVRC6},
	{416, 0, NESMirroring::MapperHVAB},
	{429, 439, NESMirroring::MapperHVAB},
	{448, 459, NESMirroring::Header},

	/* Mapper 030 */
	{476, 486, NESMirroring::UNROM512},
	{495, 439, NESMirroring::Header},
	{517, 528, NESMirroring::MapperHV},
	{533, 546, NESMirroring::MapperHV},
	{552, 0, NESMirroring::Header},
	{568, 599, NESMirroring::MapperJY},
	{612, 633, NESMirroring::MapperHV},
	{637, 6, NESMirroring::MapperHV},
	{652, 666, NESMirroring::Header},
	{676, 0, NESMirroring::Header},

	/* Mapper 040 */
	{690, 718, NESMirroring::Header},
	{724, 739, NESMirroring::MapperHV},
	{747, 0, NESMirroring::MapperHV},
	{762, 0, NESMirroring::Header},
	{637, 0, NESMirroring::MapperHV},
	{794, 0, NESMirroring::MapperHV},
	{817, 176, NESMirroring::Header},
	{637, 6, NESMirroring::MapperHV},
	{840, 546, NESMirroring::MapperHV},
	{637, 0, NESMirroring::MapperHV},

	/* Mapper 050 */
	{853, 881, NESMirroring::Header},
	{0, 0, NESMirroring::Unknown},
	{637, 0, NESMirroring::MapperHV},
	{0, 0, NESMirroring::Unknown},
	{886, 0, NESMirroring::Unknown},
	{913, 0, NESMirroring::Header},
	{931, 0, NESMirroring::MapperHV},
	{968, 0, NESMirroring::MapperHV},
	{978, 0, NESMirroring::MapperHV},
	{1002, 0, NESMirroring::MapperHV},

	/* Mapper 060 */
	{1032, 0, NESMirroring::Header},
	{1070, 0, NESMirroring::MapperHV},
	{1088, 0, NESMirroring::MapperHV},
	{1113, 718, NESMirroring::MapperHV},
	{1141, 1156, NESMirroring::MapperHV},
	{1163, 528, NESMirroring::MapperHV},
	{1174, 6, NESMirroring::Header},
	{1187, 1197, NESMirroring::MapperHVAB},
	{1205, 1197, NESMirroring::MapperSunsoft4},
	{1215, 1197, NESMirroring::MapperHVAB},

	/* Mapper 070 */
	{1229, 267, NESMirroring::Header},
	{1244, 1270, NESMirroring::Header},
	{1282, 291, NESMirroring::Header},
	{1295, 351, NESMirroring::Header},
	{1300, 1328, NESMirroring::MapperHV},
	{1336, 351, NESMirroring::MapperHV},
	{1341, 312, NESMirroring::Header},
	{1374, 1389, NESMirroring::FourScreen},
	{1395, 0, NESMirroring::Mapper},
	{1432, 1449, NESMirroring::Header},

	/* Mapper 080 */
	{1478, 546, NESMirroring::MapperHV},
	{1491, 718, NESMirroring::Header},
	{1501, 546, NESMirroring::MapperHV},
	{1548, 1548, NESMirroring::MapperHVAB},
	{1558, 0, NESMirroring::Unknown},
	{1567, 351, NESMirroring::MapperHVAB},
	{1572, 291, NESMirroring::Header},
	{1585, 0, NESMirroring::Header},
	{1599, 0, NESMirroring::Header},
	{1618, 1197, NESMirroring::MapperAB},

	/* Mapper 090 */
	{1646, 599, NESMirroring::MapperHVAB},
	{1686, 599, NESMirroring::MapperHV},
	{1719, 291, NESMirroring::Header},
	{1731, 1197, NESMirroring::Header},
	{1760, 6, NESMirroring::Header},
	{1771, 312, NESMirroring::MapperNamcot3425},
	{1783, 267, NESMirroring::Header},
	{1793, 528, NESMirroring::MapperHV},
	{0, 0, NESMirroring::Unknown},
	{1805, 6, NESMirroring::FourScreen},

	/* Mapper 100 */
	{1824, 0, NESMirroring::MapperHV},
	{1851, 1874, NESMirroring::Header},
	{0, 0, NESMirroring::Unknown},
	{1882, 0, NESMirroring::MapperHV},
	{1915, 0, NESMirroring::Header},
	{1930, 6, NESMirroring::MapperHVAB},
	{1991, 0, NESMirroring::MapperHV},
	{2021, 2034, NESMirroring::Header},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 110 */
	{0, 0, NESMirroring::Unknown},
	{2046, 2081, NESMirroring::MapperGTROM},
	{1599, 0, NESMirroring::MapperHV},
	{2100, 0, NESMirroring::MapperHV},
	{2121, 0, NESMirroring::MapperHV},
	{2154, 2194, NESMirroring::MapperHV},
	{2204, 2231, NESMirroring::MapperHVAB},
	{0, 0, NESMirroring::Unknown},
	{2238, 6, NESMirroring::MapperTxSROM},
	{2245, 6, NESMirroring::MapperHV},

	/* Mapper 120 */
	{0, 0, NESMirroring::Unknown},
	{2251, 2194, NESMirroring::MapperHV},
	{0, 0, NESMirroring::Unknown},
	{2290, 2194, NESMirroring::MapperHV},
	{0, 0, NESMirroring::Unknown},
	{2319, 2365, NESMirroring::Header},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 130 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{2380, 633, NESMirroring::Header},
	{2402, 2414, NESMirroring::Header},
	{2421, 0, NESMirroring::MapperHV},
	{0, 0, NESMirroring::Unknown},
	{2459, 2414, NESMirroring::Header},
	{2471, 2414, NESMirroring::MapperSachen8259},
	{2484, 2414, NESMirroring::MapperSachen8259},
	{2497, 2414, NESMirroring::MapperSachen8259},

	/* Mapper 140 */
	{2510, 291, NESMirroring::Header},
	{2546, 2414, NESMirroring::MapperSachen8259},
	{2559, 2590, NESMirroring::Header},
	{2597, 0, NESMirroring::Header},
	{2617, 2651, NESMirroring::Header},
	{2676, 2414, NESMirroring::Header},
	{2701, 0, NESMirroring::Header},
	{2735, 2414, NESMirroring::Header},
	{2747, 2778, NESMirroring::Header},
	{2794, 2414, NESMirroring::Header},

	/* Mapper 150 */
	{2816, 2414, NESMirroring::MapperSachen74LS374N},
	{2838, 351, NESMirroring::FourScreen},
	{2856, 2590, NESMirroring::MapperAB},
	{2886, 267, NESMirroring::MapperHVAB},
	{2916, 312, NESMirroring::MapperAB},
	{2928, 6, NESMirroring::MapperHVAB},
	{2934, 2943, NESMirroring::MapperDIS23C01},
	{2956, 267, NESMirroring::MapperHVAB},
	{2980, 1156, NESMirroring::MapperTxSROM},
	{2994, 267, NESMirroring::MapperHVAB},

	/* Mapper 160 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{3020, 3020, NESMirroring::Header},
	{3028, 1328, NESMirroring::Header},
	{3049, 0, NESMirroring::MapperHV},
	{3093, 3111, NESMirroring::Header},
	{3117, 3111, NESMirroring::Header},
	{3135, 3157, NESMirroring::Header},
	{3173, 3173, NESMirroring::Unknown},

	/* Mapper 170 */
	{0, 0, NESMirroring::Unknown},
	{3180, 2590, NESMirroring::Header},
	{3195, 0, NESMirroring::MapperHV},
	{3213, 3228, NESMirroring::Header},
	{968, 0, NESMirroring::MapperHV},
	{0, 0, NESMirroring::Unknown},
	{3237, 1328, NESMirroring::MapperHVAB},
	{676, 3268, NESMirroring::MapperHV},
	{3286, 3286, NESMirroring::MapperHV},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 180 */
	{3339, 3367, NESMirroring::Header},
	{3378, 3367, NESMirroring::Header},
	{3403, 0, NESMirroring::MapperHV},
	{3450, 0, NESMirroring::MapperHVAB},
	{3476, 1197, NESMirroring::Header},
	{3486, 0, NESMirroring::Header},
	{3518, 3528, NESMirroring::Header},
	{3544, 2194, NESMirroring::MapperHV},
	{3574, 267, NESMirroring::MapperHV},
	{3596, 0, NESMirroring::MapperHV},

	/* Mapper 190 */
	{3625, 0, NESMirroring::Header},
	{3642, 0, NESMirroring::MapperHV},
	{3642, 0, NESMirroring::MapperHV},
	{3653, 718, NESMirroring::MapperHV},
	{3642, 0, NESMirroring::MapperHV},
	{3666, 1328, NESMirroring::MapperHV},
	{3693, 0, NESMirroring::MapperHV},
	{3720, 2194, NESMirroring::MapperHV /* not sure */},
	{3743, 0, NESMirroring::MapperHV},
	{3783, 1328, NESMirroring::MapperHVAB},

	/* Mapper 200 */
	{968, 0, NESMirroring::MapperHV},
	{3827, 0, NESMirroring::Header},
	{3846, 0, NESMirroring::MapperHV},
	{3865, 0, NESMirroring::Header},
	{0, 0, NESMirroring::Unknown},
	{637, 0, NESMirroring::MapperHV},
	{3883, 6, NESMirroring::Header},
	{3918, 546, NESMirroring::MapperNamcot3425},
	{3935, 0, NESMirroring::MapperHV},
	{3979, 599, NESMirroring::MapperJY},

	/* Mapper 210 */
	{4010, 312, NESMirroring::MapperHVAB},
	{4026, 599, NESMirroring::MapperJY},
	{4068, 0, NESMirroring::MapperHV},
	{4091, 0, NESMirroring::MapperHV},
	{0, 0, NESMirroring::Unknown},
	{4129, 4155, NESMirroring::MapperHV},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{4168, 439, NESMirroring::MagicFloor},
	{4180, 2194, NESMirroring::MapperHV},

	/* Mapper 220 */
	{4209, 4237, NESMirroring::Unknown},
	{4248, 718, NESMirroring::MapperHV},
	{4262, 0, NESMirroring::MapperHVAB},
	{0, 0, NESMirroring::Unknown},
	{4283, 4297, NESMirroring::Unknown},
	{968, 0, NESMirroring::MapperHV},
	{968, 0, NESMirroring::MapperHV},
	{968, 0, NESMirroring::MapperHV},
	{4304, 4304, NESMirroring::MapperHV},
	{4323, 0, NESMirroring::MapperHV},

	/* Mapper 230 */
	{968, 0, NESMirroring::MapperHV},
	{968, 0, NESMirroring::MapperHV},
	{4335, 1270, NESMirroring::Header},
	{968, 0, NESMirroring::Mapper233},
	{4355, 0, NESMirroring::MapperHV},
	{4373, 0, NESMirroring::Mapper235},
	{4404, 4417, NESMirroring::MapperHV},
	{4425, 0, NESMirroring::MapperHV},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 240 */
	{968, 0, NESMirroring::Header},
	{4456, 0, NESMirroring::Header},
	{4487, 0, NESMirroring::MapperHV},
	{4498, 2414, NESMirroring::MapperSachen74LS374N},
	{0, 0, NESMirroring::Unknown},
	{3642, 0, NESMirroring::MapperHV},
	{4513, 4556, NESMirroring::Header},
	{0, 0, NESMirroring::Unknown},
	{4560, 2194, NESMirroring::MapperHV},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 250 */
	{4638, 4657, NESMirroring::MapperHV},
	{0, 0, NESMirroring::Unknown},
	{4663, 1328, NESMirroring::MapperHVAB},
	{4684, 1328, NESMirroring::MapperHVAB},
	{4732, 0, NESMirroring::MapperHV},
	{4760, 0, NESMirroring::MapperHV},
	{4793, 0, NESMirroring::Unknown},
	{4810, 0, NESMirroring::Unknown},
	{4823, 0, NESMirroring::Unknown},
	{4833, 0, NESMirroring::Unknown},

	/* Mapper 260 */
	{4860, 0, NESMirroring::Unknown},
	{4884, 0, NESMirroring::Unknown},
	{4911, 2414, NESMirroring::Unknown},
	{4938, 0, NESMirroring::Unknown},
	{4972, 1548, NESMirroring::Unknown},
	{4997, 0, NESMirroring::Unknown},
	{5013, 0, NESMirroring::Unknown},
	{5029, 599, NESMirroring::Unknown},
	{5066, 0, NESMirroring::Unknown},
	{5093, 0, NESMirroring::Unknown},

	/* Mapper 270 */
	{5116, 0, NESMirroring::Unknown},
	{5134, 633, NESMirroring::Unknown},
	{5165, 0, NESMirroring::Unknown},
	{5210, 0, NESMirroring::Unknown},
	{5231, 5257, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 280 */
	{0, 0, NESMirroring::Unknown},
	{5267, 599, NESMirroring::Unknown},
	{5311, 599, NESMirroring::Unknown},
	{5334, 0, NESMirroring::Unknown},
	{5371, 439, NESMirroring::Unknown},
	{5376, 0, NESMirroring::Unknown},
	{5392, 5412, NESMirroring::Unknown},
	{5422, 0, NESMirroring::Unknown},
	{5460, 0, NESMirroring::Unknown},
	{5484, 0, NESMirroring::Unknown},

	/* Mapper 290 */
	{5495, 5519, NESMirroring::Unknown},
	{5525, 2194, NESMirroring::Unknown},
	{5558, 0, NESMirroring::Unknown},
	{5586, 0, NESMirroring::Unknown},
	{5620, 0, NESMirroring::Unknown},
	{5672, 599, NESMirroring::Unknown},
	{5703, 0, NESMirroring::Unknown},
	{5749, 633, NESMirroring::Unknown},
	{5776, 0, NESMirroring::Unknown},
	{5816, 633, NESMirroring::Unknown},

	/* Mapper 300 */
	{5847, 0, NESMirroring::Unknown},
	{5873, 0, NESMirroring::Unknown},
	{5887, 2590, NESMirroring::Unknown},
	{5911, 2590, NESMirroring::Unknown},
	{747, 2365, NESMirroring::Unknown},
	{5945, 2590, NESMirroring::Unknown},
	{5989, 2590, NESMirroring::Unknown},
	{6022, 2590, NESMirroring::Unknown},
	{6047, 0, NESMirroring::Unknown},
	{6087, 2365, NESMirroring::Unknown},

	/* Mapper 310 */
	{6120, 2365, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{6180, 2590, NESMirroring::Unknown},
	{6203, 0, NESMirroring::Unknown},
	{6232, 0, NESMirroring::Unknown},
	{6246, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 320 */
	{6277, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{6310, 0, NESMirroring::Unknown},
	{6337, 0, NESMirroring::Unknown},
	{6377, 0, NESMirroring::Unknown},
	{6417, 0, NESMirroring::Unknown},
	{6450, 0, NESMirroring::Unknown},
	{6474, 0, NESMirroring::Unknown},
	{6491, 0, NESMirroring::Unknown},
	{6548, 0, NESMirroring::Unknown},

	/* Mapper 330 */
	{6572, 0, NESMirroring::Unknown},
	{6613, 0, NESMirroring::Unknown},
	{6637, 0, NESMirroring::Unknown},
	{6661, 6693, NESMirroring::Unknown},
	{6702, 0, NESMirroring::Unknown},
	{6737, 0, NESMirroring::Unknown},
	{6755, 0, NESMirroring::Unknown},
	{6773, 0, NESMirroring::Unknown},
	{6801, 0, NESMirroring::Unknown},
	{6842, 0, NESMirroring::Unknown},

	/* Mapper 340 */
	{3865, 0, NESMirroring::Unknown},
	{6860, 0, NESMirroring::Unknown},
	{6884, 439, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{6914, 0, NESMirroring::Unknown},
	{6965, 6693, NESMirroring::Unknown},
	{7006, 2590, NESMirroring::Unknown},
	{7029, 2590, NESMirroring::Unknown},
	{7075, 0, NESMirroring::Unknown},
	{7083, 0, NESMirroring::Unknown},

	/* Mapper 350 */
	{7124, 0, NESMirroring::Unknown},
	{7158, 7175, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{7199, 0, NESMirroring::Unknown},
	{7231, 0, NESMirroring::Unknown},
	{7250, 0, NESMirroring::Unknown},
	{7269, 599, NESMirroring::Unknown},
	{7293, 666, NESMirroring::Unknown},
	{5311, 599, NESMirroring::Unknown},
	{7317, 0, NESMirroring::Unknown},

	/* Mapper 360 */
	{7355, 666, NESMirroring::Unknown},
	{7380, 599, NESMirroring::Unknown},
	{7413, 599, NESMirroring::Unknown},
	{5311, 599, NESMirroring::Unknown},
	{7445, 599, NESMirroring::Unknown},
	{7465, 5519, NESMirroring::Unknown},
	{7498, 0, NESMirroring::Unknown},
	{7527, 0, NESMirroring::Unknown},
	{7544, 7585, NESMirroring::Unknown},
	{7593, 0, NESMirroring::Unknown},

	/* Mapper 370 */
	{7602, 0, NESMirroring::Unknown},
	{7607, 7647, NESMirroring::Unknown},
	{7654, 0, NESMirroring::Unknown},
	{7685, 0, NESMirroring::Unknown},
	{7717, 0, NESMirroring::Unknown},
	{7744, 0, NESMirroring::Unknown},
	{7771, 599, NESMirroring::Unknown},
	{7791, 599, NESMirroring::Unknown},
	{7823, 0, NESMirroring::Unknown},
	{7852, 0, NESMirroring::Unknown},

	/* Mapper 380 */
	{7875, 0, NESMirroring::Unknown},
	{7883, 0, NESMirroring::Unknown},
	{7889, 0, NESMirroring::Unknown},
	{7897, 599, NESMirroring::Unknown},
	{7920, 0, NESMirroring::Unknown},
	{7940, 718, NESMirroring::Unknown},
	{7951, 599, NESMirroring::Unknown},
	{7961, 599, NESMirroring::Unknown},
	{7983, 599, NESMirroring::Unknown},
	{8005, 739, NESMirroring::Unknown},

	/* Mapper 390 */
	{8030, 4417, NESMirroring::Unknown},
	{8043, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 400 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 410 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 420 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 430 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 440 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 450 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 460 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 470 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 480 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 490 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 500 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 510 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{8064, 2414, NESMirroring::Unknown},
	{8084, 2414, NESMirroring::Unknown},
	{8120, 3111, NESMirroring::Unknown},
	{8134, 0, NESMirroring::Unknown},
	{8151, 8177, NESMirroring::Unknown},
	{8185, 0, NESMirroring::Unknown},
	{8208, 3111, NESMirroring::Unknown},
	{8224, 0, NESMirroring::Unknown},

	/* Mapper 520 */
	{8236, 0, NESMirroring::Unknown},
	{8274, 0, NESMirroring::Unknown},
	{8285, 2365, NESMirroring::Unknown},
	{8319, 4297, NESMirroring::Unknown},
	{8365, 0, NESMirroring::Unknown},
	{8401, 2590, NESMirroring::Unknown},
	{8427, 0, NESMirroring::Unknown},
	{8466, 0, NESMirroring::Unknown},
	{8507, 0, NESMirroring::Unknown},
	{8546, 0, NESMirroring::Unknown},

	/* Mapper 530 */
	{8591, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{8635, 2414, NESMirroring::Unknown},
	{8647, 0, NESMirroring::Unknown},
	{8689, 2365, NESMirroring::Unknown},
	{8726, 1328, NESMirroring::Unknown},
	{8726, 1328, NESMirroring::Unknown},
	{8767, 0, NESMirroring::Unknown},
	{8779, 0, NESMirroring::Unknown},

	/* Mapper 540 */
	{8807, 0, NESMirroring::Unknown},
	{8848, 0, NESMirroring::Unknown},
	{8877, 0, NESMirroring::Unknown},
	{8906, 0, NESMirroring::Unknown},
	{8945, 1328, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{8959, 351, NESMirroring::Unknown},
	{8985, 8992, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 550 */
	{0, 0, NESMirroring::Unknown},
	{9004, 4297, NESMirroring::Unknown},
	{9043, 546, NESMirroring::Unknown},
};

