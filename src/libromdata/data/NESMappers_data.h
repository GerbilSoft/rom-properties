/** NES Mappers (generated from NESMappers_data.txt) **/

#include <stdint.h>

static const char NESMappers_strtbl[] =
	"\x00" "NROM" "\x00" "Nintendo" "\x00" "SxROM (MMC1)" "\x00" "UxR"
	"OM" "\x00" "CNROM" "\x00" "TxROM (MMC3), HKROM (MMC6)" "\x00" "E"
	"xROM (MMC5)" "\x00" "Front Fareast (Game Doctor) Magic Card 1M/2"
	"M RAM cartridge" "\x00" "Bung/FFE" "\x00" "AxROM" "\x00" "Front "
	"Fareast (Game Doctor) GxROM (clone of 006, submapper 4)" "\x00" "P"
	"xROM, PEEOROM (MMC2)" "\x00" "FxROM (MMC4)" "\x00" "Color Dreams"
	"\x00" "MMC3 variant" "\x00" "FFE" "\x00" "NES-CPROM" "\x00" "SL-"
	"1632 (MMC3/VRC2 clone)" "\x00" "K-1029 (multicart)" "\x00" "FCG-"
	"x" "\x00" "Bandai" "\x00" "FFE #17" "\x00" "SS 88006" "\x00" "Ja"
	"leco" "\x00" "Namco 129/163" "\x00" "Namco" "\x00" "Famicom Disk"
	" System" "\x00" "VRC4a, VRC4c" "\x00" "Konami" "\x00" "VRC2a" "\x00"
	"VRC4e, VRC4f, VRC2b" "\x00" "VRC6a" "\x00" "VRC4b, VRC4d, VRC2c" "\x00"
	"VRC6b" "\x00" "VRC4 variant" "\x00" "Action 53" "\x00" "Homebrew"
	"\x00" "RET-CUFROM" "\x00" "Sealie Computing" "\x00" "UNROM 512" "\x00"
	"RetroUSB" "\x00" "NSF Music Compilation" "\x00" "Irem G-101" "\x00"
	"Irem" "\x00" "Taito TC0190" "\x00" "Taito" "\x00" "BNROM, NINA-0"
	"01" "\x00" "J.Y. Company ASIC (8 KiB WRAM)" "\x00" "J.Y. Company"
	"\x00" "TXC PCB 01-22000-400" "\x00" "TXC" "\x00" "MMC3 multicart"
	"\x00" "GNROM variant" "\x00" "Bit Corp." "\x00" "BNROM variant" "\x00"
	"NTDEC 2722 (FDS conversion)" "\x00" "NTDEC" "\x00" "Caltron 6-in"
	"-1" "\x00" "Caltron" "\x00" "FDS conversion" "\x00" "TONY-I, YS-"
	"612 (FDS conversion)" "\x00" "MMC3 multicart (GA23C)" "\x00" "Ru"
	"mble Station 15-in-1" "\x00" "Taito TC0690" "\x00" "PCB 761214 ("
	"FDS conversion)" "\x00" "N-32" "\x00" "11-in-1 Ball Games" "\x00"
	"Supervision 16-in-1" "\x00" "Novel Diamond 9999999-in-1" "\x00" "B"
	"TL-MARIO1-MALEE2" "\x00" "KS202 (unlicensed SMB3 reproduction)" "\x00"
	"Multicart" "\x00" "(C)NROM-based multicart" "\x00" "BMC-T3H53/BM"
	"C-D1038 multicart" "\x00" "Reset-based NROM-128 4-in-1 multicart"
	"\x00" "20-in-1 multicart" "\x00" "Super 700-in-1 multicart" "\x00"
	"Powerful 250-in-1 multicart" "\x00" "Tengen RAMBO-1" "\x00" "Ten"
	"gen" "\x00" "Irem H3001" "\x00" "GxROM, MHROM" "\x00" "Sunsoft-3"
	"\x00" "Sunsoft" "\x00" "Sunsoft-4" "\x00" "Sunsoft FME-7" "\x00" "F"
	"amily Trainer" "\x00" "Codemasters (UNROM clone)" "\x00" "Codema"
	"sters" "\x00" "Jaleco JF-17" "\x00" "VRC3" "\x00" "43-393/860908"
	"C (MMC3 clone)" "\x00" "W" "\xc3\xa0" "ix" "\xc4\xab" "ng" "\x00"
	"VRC1" "\x00" "NAMCOT-3446 (Namcot 108 variant)" "\x00" "Napoleon"
	" Senki" "\x00" "Lenar" "\x00" "Holy Diver; Uchuusen - Cosmo Carr"
	"ier" "\x00" "NINA-03, NINA-06" "\x00" "American Video Entertainm"
	"ent" "\x00" "Taito X1-005" "\x00" "Super Gun" "\x00" "Taito X1-0"
	"17 (incorrect PRG ROM bank ordering)" "\x00" "Cony/Yoko" "\x00" "P"
	"C-SMB2J" "\x00" "VRC7" "\x00" "Jaleco JF-13" "\x00" "CNROM varia"
	"nt" "\x00" "Namcot 118 variant" "\x00" "Sunsoft-2 (Sunsoft-3 boa"
	"rd)" "\x00" "J.Y. Company (simple nametable control)" "\x00" "J."
	"Y. Company (Super Fighter III)" "\x00" "Moero!! Pro" "\x00" "Sun"
	"soft-2 (Sunsoft-3R board)" "\x00" "HVC-UN1ROM" "\x00" "NAMCOT-34"
	"25" "\x00" "Oeka Kids" "\x00" "Irem TAM-S1" "\x00" "CNROM (Vs. S"
	"ystem)" "\x00" "MMC3 variant (hacked ROMs)" "\x00" "Jaleco JF-10"
	" (misdump)" "\x00" "Jaleceo" "\x00" "Drip" "\x00" "Doki Doki Pan"
	"ic (FDS conversion)" "\x00" "PEGASUS 5 IN 1" "\x00" "NES-EVENT ("
	"MMC1 variant) (Nintendo World Championships 1990)" "\x00" "Super"
	" Mario Bros. 3 (bootleg)" "\x00" "Magic Dragon" "\x00" "Magicser"
	"ies" "\x00" "FDS conversions" "\x00" "The Great Wall (Sachen 825"
	"9D) (duplicate of 137)" "\x00" "Sachen" "\x00" "Honey Peach (Sac"
	"hen SA-020A) (duplicate of 243)" "\x00" "Cheapocabra GTROM 512k "
	"flash board" "\x00" "Membler Industries" "\x00" "NINA-03/06 mult"
	"icart" "\x00" "MMC3 clone (scrambled registers)" "\x00" "K" "\xc7"
	"\x8e" "sh" "\xc3\xa8" "ng SFC-02B/-03/-004 (MMC3 clone)" "\x00" "K"
	"\xc7\x8e" "sh" "\xc3\xa8" "ng" "\x00" "SOMARI-P (Huang-1/Huang-2"
	")" "\x00" "Gouder" "\x00" "Future Media" "\x00" "TxSROM" "\x00" "T"
	"QROM" "\x00" "Tobidase Daisakusen (FDS conversion)" "\x00" "K" "\xc7"
	"\x8e" "sh" "\xc3\xa8" "ng A9711 and A9713 (MMC3 clone)" "\x00" "S"
	"unsoft-1 (duplicate of 184)" "\x00" "K" "\xc7\x8e" "sh" "\xc3\xa8"
	"ng H2288 (MMC3 clone)" "\x00" "Super Game Mega Type III" "\x00" "M"
	"onty no Doki Doki Daisass" "\xc5\x8d" " (FDS conversion)" "\x00" "W"
	"hirlwind Manu" "\x00" "Double Dragon - The Revenge (Japan) (pira"
	"ted version)" "\x00" "Super HiK 4-in-1 multicart" "\x00" "(C)NRO"
	"M-based multicart (duplicate of 058)" "\x00" "7-in-1 (NS03) mult"
	"icart (duplicate of 331)" "\x00" "MMC3 multicart (duplicate of 2"
	"05)" "\x00" "TXC 05-00002-010 ASIC" "\x00" "Jovial Race" "\x00" "T"
	"4A54A, WX-KB4K, BS-5652 (MMC3 clone)" "\x00" "Sachen 8259A varia"
	"nt (duplicate of 141)" "\x00" "Sachen 3011" "\x00" "Sachen 8259D"
	"\x00" "Sachen 8259B" "\x00" "Sachen 8259C" "\x00" "Jaleco JF-11,"
	" JF-14 (GNROM variant)" "\x00" "Sachen 8259A" "\x00" "Kaiser KS2"
	"02 (FDS conversions)" "\x00" "Kaiser" "\x00" "Copy-protected NRO"
	"M" "\x00" "Death Race (Color Dreams variant)" "\x00" "American G"
	"ame Cartridges" "\x00" "Sidewinder (CNROM clone)" "\x00" "Galact"
	"ic Crusader (NINA-06 clone)" "\x00" "Sachen 3018" "\x00" "Sachen"
	" SA-008-A, Tengen 800008" "\x00" "Sachen / Tengen" "\x00" "SA-00"
	"36 (CNROM clone)" "\x00" "Sachen SA-015, SA-630" "\x00" "VRC1 (V"
	"s. System)" "\x00" "Kaiser KS202 (FDS conversion)" "\x00" "Banda"
	"i FCG: LZ93D50 with SRAM" "\x00" "NAMCOT-3453" "\x00" "MMC1A" "\x00"
	"DIS23C01" "\x00" "Daou Infosys" "\x00" "Datach Joint ROM System" "\x00"
	"Tengen 800037" "\x00" "Bandai LZ93D50 with 24C01" "\x00" "J.Y. C"
	"ompany (simple nametable control) (duplicate of 090)" "\x00" "Ha"
	"njuku Hero (MMC1) (should be 001)" "\x00" "W" "\xc3\xa0" "ix" "\xc4"
	"\xab" "ng FS304" "\x00" "Nanjing" "\x00" "PEC-9588 Pyramid Educa"
	"tional Computer" "\x00" "D" "\xc5\x8d" "ngd" "\xc3\xa1\x00" "Fir"
	"e Emblem (unlicensed) (MMC2+MMC3 hybrid)" "\x00" "Subor (variant"
	" 1)" "\x00" "Subor" "\x00" "Subor (variant 2)" "\x00" "Racermate"
	" Challenge 2" "\x00" "Racermate, Inc." "\x00" "Yuxing" "\x00" "S"
	"hiko Game Syu" "\x00" "Kaiser KS-7058" "\x00" "Super Mega P-4040"
	"\x00" "Idea-Tek ET-xx" "\x00" "Idea-Tek" "\x00" "Kaiser 15-in-1 "
	"multicart" "\x00" "W" "\xc3\xa0" "ix" "\xc4\xab" "ng multicart ("
	"8025) (MMC3 clone)" "\x00" "H" "\xc3\xa9" "ngg" "\xc3\xa9" " Di" "\xc3"
	"\xa0" "nz" "\xc7\x90\x00" "W" "\xc3\xa0" "ix" "\xc4\xab" "ng / N"
	"anjing / Jncota / Henge Dianzi / GameStar" "\x00" "W" "\xc3\xa0" "i"
	"x" "\xc4\xab" "ng multicart (8025) (MMC3 clone) (duplicate of 17"
	"6)" "\x00" "Crazy Climber (UNROM clone)" "\x00" "Nichibutsu" "\x00"
	"Seicross v2 (FCEUX hack)" "\x00" "MMC3 clone (scrambled register"
	"s) (same as 114)" "\x00" "Suikan Pipe (VRC4e clone)" "\x00" "Sun"
	"soft-1" "\x00" "CNROM with weak copy protection" "\x00" "Study B"
	"ox" "\x00" "Fukutake Shoten" "\x00" "K" "\xc7\x8e" "sh" "\xc3\xa8"
	"ng A98402 (MMC3 clone)" "\x00" "Bandai Karaoke Studio" "\x00" "T"
	"hunder Warrior (MMC3 clone)" "\x00" "Magic Kid GooGoo" "\x00" "M"
	"MC3 clone" "\x00" "NTDEC TC-112" "\x00" "W" "\xc3\xa0" "ix" "\xc4"
	"\xab" "ng FS303 (MMC3 clone)" "\x00" "Mario bootleg (MMC3 clone)"
	"\x00" "K" "\xc7\x8e" "sh" "\xc3\xa8" "ng (MMC3 clone)" "\x00" "T"
	"\xc5\xab" "nsh" "\xc3\xad" " Ti" "\xc4\x81" "nd" "\xc3\xac" " - "
	"S" "\xc4\x81" "ngu" "\xc3\xb3" " W" "\xc3\xa0" "izhu" "\xc3\xa0" "n"
	"\x00" "W" "\xc3\xa0" "ix" "\xc4\xab" "ng (clone of either Mapper"
	" 004 or 176)" "\x00" "NROM-256 multicart" "\x00" "150-in-1 multi"
	"cart" "\x00" "35-in-1 multicart" "\x00" "DxROM (Tengen MIMIC-1, "
	"Namcot 118)" "\x00" "Fudou Myouou Den" "\x00" "Street Fighter IV"
	" (unlicensed) (MMC3 clone)" "\x00" "J.Y. Company (MMC2/MMC4 clon"
	"e)" "\x00" "Namcot 175, 340" "\x00" "J.Y. Company (extended name"
	"table control)" "\x00" "BMC Super HiK 300-in-1" "\x00" "(C)NROM-"
	"based multicart (same as 058)" "\x00" "Super Gun 20-in-1 multica"
	"rt" "\x00" "Sugar Softec (MMC3 clone)" "\x00" "Sugar Softec" "\x00"
	"Bonza, Magic Jewelry II (Russian; Dendy)" "\x00" "500-in-1 / 200"
	"0-in-1 multicart" "\x00" "Magic Floor" "\x00" "K" "\xc7\x8e" "sh"
	"\xc3\xa8" "ng A9461 (MMC3 clone)" "\x00" "Summer Carnival '92 - "
	"Recca" "\x00" "Naxat Soft" "\x00" "NTDEC N625092" "\x00" "CTC-31"
	" (VRC2 + 74xx)" "\x00" "W" "\xc3\xa0" "ix" "\xc4\xab" "ng (clone"
	" of either Mapper 004 or 176) (duplicate of 199)" "\x00" "Jncota"
	" KT-008" "\x00" "Jncota" "\x00" "Active Enterprises" "\x00" "BMC"
	" 31-IN-1" "\x00" "Codemasters Quattro" "\x00" "Maxi 15 multicart"
	"\x00" "Golden Game 150-in-1 multicart" "\x00" "Realtec 8155" "\x00"
	"Realtec" "\x00" "Teletubbies 420-in-1 multicart" "\x00" "Contra "
	"Fighter (G.I. Joe w/ SF2 characters hack)" "\x00" "BNROM variant"
	" (similar to 034)" "\x00" "Unlicensed" "\x00" "Sachen SA-020A" "\x00"
	"Decathlon" "\x00" "C&E" "\x00" "F" "\xc4\x93" "ngsh" "\xc3\xa9" "n"
	"b" "\xc7\x8e" "ng: F" "\xc3\xba" "m" "\xc3\xb3" " S" "\xc4\x81" "n"
	" T" "\xc3\xa0" "iz" "\xc7\x90" " (C&E)" "\x00" "K" "\xc7\x8e" "s"
	"h" "\xc3\xa8" "ng SFC-02B/-03/-004 (MMC3 clone) (incorrect assig"
	"nment; should be 115)" "\x00" "W" "\xc3\xa0" "ix" "\xc4\xab" "ng"
	" MMC3 TKROM clone with address scrambling" "\x00" "Nitra (MMC3 c"
	"lone)" "\x00" "Nitra" "\x00" "MMC3 multicart (GA23C) (duplicate "
	"of 045)" "\x00" "W" "\xc3\xa0" "ix" "\xc4\xab" "ng - Sangokushi" "\x00"
	"Dragon Ball Z: Ky" "\xc5\x8d" "sh" "\xc5\xab" "! Saiya-jin (VRC4"
	" clone)" "\x00" "Pikachu Y2K of crypted ROMs" "\x00" "110-in-1 m"
	"ulticart (same as 225)" "\x00" "OneBus Famiclone" "\x00" "UNIF P"
	"EC-586" "\x00" "UNIF 158B" "\x00" "UNIF F-15 (MMC3 multicart)" "\x00"
	"HP10xx/HP20xx multicart" "\x00" "200-in-1 Elfland multicart" "\x00"
	"Street Heroes (MMC3 clone)" "\x00" "King of Fighters '97 (MMC3 c"
	"lone)" "\x00" "Cony/Yoko Fighting Games" "\x00" "T-262 multicart"
	"\x00" "City Fighter IV" "\x00" "8-in-1 JY-119 multicart (MMC3 cl"
	"one)" "\x00" "SMD132/SMD133 (MMC3 clone)" "\x00" "Multicart (MMC"
	"3 clone)" "\x00" "Game Prince RS-16" "\x00" "TXC 4-in-1 multicar"
	"t (MGC-026)" "\x00" "Akumaj" "\xc5\x8d" " Special: Boku Dracula-"
	"kun (bootleg)" "\x00" "Gremlins 2 (bootleg)" "\x00" "Cartridge S"
	"tory multicart" "\x00" "RCM Group" "\x00" "J.Y. Company Super Hi"
	"K 3/4/5-in-1 multicart" "\x00" "J.Y. Company multicart" "\x00" "B"
	"lock Family 6-in-1/7-in-1 multicart" "\x00" "A65AS multicart" "\x00"
	"Benshieng multicart" "\x00" "Benshieng" "\x00" "4-in-1 multicart"
	" (411120-C, 811120-C)" "\x00" "GKCX1 21-in-1 multicart" "\x00" "B"
	"MC-60311C" "\x00" "Asder 20-in-1 multicart" "\x00" "Asder" "\x00"
	"K" "\xc7\x8e" "sh" "\xc3\xa8" "ng 2-in-1 multicart (MK6)" "\x00" "D"
	"ragon Fighter (unlicensed)" "\x00" "NewStar 12-in-1/76-in-1 mult"
	"icart" "\x00" "T4A54A, WX-KB4K, BS-5652 (MMC3 clone) (same as 13"
	"4)" "\x00" "J.Y. Company 13-in-1 multicart" "\x00" "FC Pocket RS"
	"-20 / dreamGEAR My Arcade Gamer V" "\x00" "TXC 01-22110-000 mult"
	"icart" "\x00" "Lethal Weapon (unlicensed) (VRC4 clone)" "\x00" "T"
	"XC 6-in-1 multicart (MGC-023)" "\x00" "Golden 190-in-1 multicart"
	"\x00" "GG1 multicart" "\x00" "Gyruss (FDS conversion)" "\x00" "A"
	"lmana no Kiseki (FDS conversion)" "\x00" "Dracula II: Noroi no F"
	"\xc5\xab" "in (FDS conversion)" "\x00" "Exciting Basket (FDS con"
	"version)" "\x00" "Metroid (FDS conversion)" "\x00" "Batman (Suns"
	"oft) (bootleg) (VRC2 clone)" "\x00" "Ai Senshi Nicol (FDS conver"
	"sion)" "\x00" "Monty no Doki Doki Daisass" "\xc5\x8d" " (FDS con"
	"version) (same as 125)" "\x00" "Super Mario Bros. 2 pirate cart "
	"(duplicate of 043)" "\x00" "Highway Star (bootleg)" "\x00" "Rese"
	"t-based multicart (MMC3)" "\x00" "Y2K multicart" "\x00" "820732C"
	"- or 830134C- multicart" "\x00" "HP-898F / KD-7/9-E boards" "\x00"
	"Super HiK 6-in-1 A-030 multicart" "\x00" "35-in-1 (K-3033) multi"
	"cart" "\x00" "Farid's homebrew 8-in-1 SLROM multicart" "\x00" "F"
	"arid's homebrew 8-in-1 UNROM multicart" "\x00" "Super Mali Splas"
	"h Bomb (bootleg)" "\x00" "Contra/Gryzor (bootleg)" "\x00" "6-in-"
	"1 multicart" "\x00" "Test Ver. 1.01 Dlya Proverki TV Pristavok t"
	"est cartridge" "\x00" "Education Computer 2000" "\x00" "Sangokus"
	"hi II: Ha" "\xc5\x8d" " no Tairiku (bootleg)" "\x00" "7-in-1 (NS"
	"03) multicart" "\x00" "Super 40-in-1 multicart" "\x00" "New Star"
	" Super 8-in-1 multicart" "\x00" "New Star" "\x00" "5/20-in-1 199"
	"3 Copyright multicart" "\x00" "10-in-1 multicart" "\x00" "11-in-"
	"1 multicart" "\x00" "12-in-1 Game Card multicart" "\x00" "16-in-"
	"1, 200/300/600/1000-in-1 multicart" "\x00" "21-in-1 multicart" "\x00"
	"Simple 4-in-1 multicart" "\x00" "COOLGIRL multicart (Homebrew)" "\x00"
	"Kuai Da Jin Ka Zhong Ji Tiao Zhan 3-in-1 multicart" "\x00" "New "
	"Star 6-in-1 Game Cartridge multicart" "\x00" "Zanac (FDS convers"
	"ion)" "\x00" "Yume Koujou: Doki Doki Panic (FDS conversion)" "\x00"
	"830118C" "\x00" "1994 Super HIK 14-in-1 (G-136) multicart" "\x00"
	"Super 15-in-1 Game Card multicart" "\x00" "9-in-1 multicart" "\x00"
	"J.Y. Company / Techline" "\x00" "Kaiser 4-in-1 multicart (KS106C"
	")" "\x00" "92 Super Mario Family multicart" "\x00" "250-in-1 mul"
	"ticart" "\x00\xe9\xbb\x83\xe4\xbf\xa1\xe7\xb6\xad" " 3D-BLOCK" "\x00"
	"7-in-1 Rockman (JY-208)" "\x00" "4-in-1 (4602) multicart" "\x00" "S"
	"B-5013 / GCL8050 / 841242C multicart" "\x00" "31-in-1 (3150) mul"
	"ticart" "\x00" "YY841101C multicart (MMC3 clone)" "\x00" "830506"
	"C multicart (VRC4f clone)" "\x00" "JY830832C multicart" "\x00" "A"
	"sder PC-95 educational computer" "\x00" "GN-45 multicart (MMC3 c"
	"lone)" "\x00" "7-in-1 multicart" "\x00" "Super Mario Bros. 2 (J)"
	" (FDS conversion)" "\x00" "YUNG-08" "\x00" "N49C-300" "\x00" "F6"
	"00" "\x00" "Spanish PEC-586 home computer cartridge" "\x00" "Don"
	"gda" "\x00" "Rockman 1-6 (SFC-12) multicart" "\x00" "Super 4-in-"
	"1 (SFC-13) multicart" "\x00" "Reset-based MMC1 multicart" "\x00" "1"
	"35-in-1 (U)NROM multicart" "\x00" "YY841155C multicart" "\x00" "1"
	"998 Super Game 8-in-1 (JY-111)" "\x00" "8-in-1 AxROM/UNROM multi"
	"cart" "\x00" "35-in-1 NROM multicart" "\x00" "970630C" "\x00" "K"
	"N-42" "\x00" "830928C" "\x00" "YY840708C (MMC3 clone)" "\x00" "L"
	"1A16 (VRC4e clone)" "\x00" "NTDEC 2779" "\x00" "YY860729C" "\x00"
	"YY850735C / YY850817C" "\x00" "YY841145C / YY850835C" "\x00" "Ca"
	"ltron 9-in-1 multicart" "\x00" "Realtec 8031" "\x00" "NC7000M (M"
	"MC3 clone)" "\x00" "Realtec HSK007 multicart" "\x00" "Realtec 82"
	"10 multicart (MMC3-compatible)" "\x00" "YY850437C / Realtec GN-5"
	"1 multicart" "\x00" "1996 Soccer 6-in-1 (JY-082)" "\x00" "YY8408"
	"20C" "\x00" "Star Versus" "\x00" "8-BIT XMAS 2017" "\x00" "retro"
	"USB" "\x00" "KC885 multicart" "\x00" "J-2282 multicart" "\x00" "8"
	"9433" "\x00" "JY012005 MMC1-based multicart" "\x00" "Game design"
	"ed for UMC UM6578" "\x00" "UMC" "\x00" "Haradius Zero" "\x00" "W"
	"in, Lose, or Draw Plug-n-Play (VT03)" "\x00" "Konami Collector's"
	" Series Advance Arcade Plug & Play" "\x00" "DPCMcart" "\x00" "Su"
	"per 8-in-1 multicart (JY-302)" "\x00" "A88S-1 multicart (MMC3-co"
	"mpatible)" "\x00" "Intellivision 10-in-1 Plug 'n Play 2nd Editio"
	"n" "\x00" "Techno Source" "\x00" "Super Russian Roulette" "\x00" "9"
	"999999-in-1 multicart" "\x00" "Lucky Rabbit (0353) (FDS conversi"
	"on of Roger Rabbit)" "\x00" "4-in-1 multicart" "\x00" "Batman: T"
	"he Video Game (Fine Studio bootleg)" "\x00" "Fine Studio" "\x00" "8"
	"20106-C / 821007C (LH42)" "\x00" "TK-8007 MCU (VT03)" "\x00" "Ta"
	"ikee" "\x00" "A971210" "\x00" "Kasheng" "\x00" "SC871115C" "\x00"
	"BS-300 / BS-400 / BS-4040 multicarts" "\x00" "Lexibook Compact C"
	"yber Arcade (VT369)" "\x00" "Lexibook" "\x00" "Lexibook Retro TV"
	" Game Console (VT369)" "\x00" "Cube Tech VT369 handheld consoles"
	"\x00" "Cube Tech" "\x00" "VT369-based handheld console with 256-"
	"byte serial ROM" "\x00" "VT369-based handheld console with I" "\xc2"
	"\xb2" "C protection chip" "\x00" "(C)NROM multicart" "\x00" "Mil"
	"owork FCFC1 flash cartridge (A mode)" "\x00" "Milowork" "\x00" "8"
	"31031C/T-308 multicart (MMC3-based)" "\x00" "Realtec GN-91B" "\x00"
	"Realtec 8090 (MMC3-based)" "\x00" "NC-20MB PCB; 20-in-1 multicar"
	"t (CA-006)" "\x00" "S-009" "\x00" "NC3000M multicart" "\x00" "NC"
	"7000M multicart (MMC3-compatible)" "\x00" "DG574B multicart (MMC"
	"3-compatible)" "\x00" "SMD172B_FPGA" "\x00" "Mindkids" "\x00" "K"
	"L-06 multicart (VRC4)" "\x00" "830768C multicart (VRC4)" "\x00" "S"
	"uper Games King" "\x00" "YY841157C multicart (VRC2)" "\x00" "Har"
	"atyler HP/MP" "\x00" "DS-9-27 multicart" "\x00" "Realtec 8042" "\x00"
	"47-2 (MMC3)" "\x00" "Zh" "\xc5\x8d" "nggu" "\xc3\xb3" " D" "\xc3"
	"\xa0" "h" "\xc4\x93" "ng" "\x00" "M" "\xc4\x9b" "i Sh" "\xc3\xa0"
	"on" "\xc7\x9a" " M" "\xc3\xa8" "ng G" "\xc5\x8d" "ngch" "\xc7\x8e"
	"ng III" "\x00" "Subor Karaoke" "\x00" "Family Noraebang" "\x00" "B"
	"rilliant Com Cocoma Pack" "\x00" "EduBank" "\x00" "Kkachi-wa Nol"
	"ae Chingu" "\x00" "Subor multicart" "\x00" "UNL-EH8813A" "\x00" "2"
	"-in-1 Datach multicart (VRC4e clone)" "\x00" "Korean Igo" "\x00" "F"
	"\xc5\xab" "un Sh" "\xc5\x8d" "rinken (FDS conversion)" "\x00" "F"
	"\xc4\x93" "ngsh" "\xc3\xa9" "nb" "\xc7\x8e" "ng: F" "\xc3\xba" "m"
	"\xc3\xb3" " S" "\xc4\x81" "n T" "\xc3\xa0" "iz" "\xc7\x90" " (Jn"
	"cota)" "\x00" "The Lord of King (Jaleco) (bootleg)" "\x00" "UNL-"
	"KS7021A (VRC2b clone)" "\x00" "Sangokushi: Ch" "\xc5\xab" "gen n"
	"o Hasha (bootleg)" "\x00" "Fud" "\xc5\x8d" " My" "\xc5\x8d\xc5\x8d"
	" Den (bootleg) (VRC2b clone)" "\x00" "1995 New Series Super 2-in"
	"-1 multicart" "\x00" "Datach Dragon Ball Z (bootleg) (VRC4e clon"
	"e)" "\x00" "Super Mario Bros. Pocker Mali (VRC4f clone)" "\x00" "L"
	"ittleCom PC-95" "\x00" "CHINA_ER_SAN2" "\x00" "Sachen 3014" "\x00"
	"2-in-1 Sudoku/Gomoku (NJ064) (MMC3 clone)" "\x00" "Nazo no Muras"
	"amej" "\xc5\x8d" " (FDS conversion)" "\x00" "W" "\xc3\xa0" "ix" "\xc4"
	"\xab" "ng FS303 (MMC3 clone) (same as 195)" "\x00" "60-1064-16L" "\x00"
	"Kid Icarus (FDS conversion)" "\x00" "Master Fighter VI' hack (va"
	"riant of 359)" "\x00" "LittleCom 160-in-1 multicart" "\x00" "Wor"
	"ld Hero hack (VRC4 clone)" "\x00" "5-in-1 (CH-501) multicart (MM"
	"C1 clone)" "\x00" "W" "\xc3\xa0" "ix" "\xc4\xab" "ng FS306" "\x00"
	"ST-80 (4-in-1)" "\x00" "10-in-1 Tenchi wo Kurau multicart" "\x00"
	"Konami QTa adapter (VRC5)" "\x00" "CTC-15" "\x00" "Co Tung Co." "\x00"
	"Meikyuu Jiin Dababa" "\x00" "JY820845C" "\x00" "Jncota RPG re-re"
	"lease (variant of 178)" "\x00" "Taito X1-017 (correct PRG ROM ba"
	"nk ordering)" "\x00" "Sachen 3013" "\x00" "Kaiser KS-7010" "\x00"
	"Nintendo Campus Challenge 1991 (RetroUSB version)" "\x00" "JY-21"
	"5 multicart" "\x00" "Moero TwinBee: Cinnamon-hakase o Sukue! (FD"
	"S conversion)" "\x00" "YC-03-09" "\x00" "Y" "\xc3\xa0" "nch" "\xc3"
	"\xa9" "ng" "\x00" "Bung Super Game Doctor 2M/4M RAM cartridge" "\x00"
	"Venus Turbo Game Doctor 4+/6+/6M RAM cartridge" "\x00";

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
	{80, 139, NESMirroring::MapperHVAB},
	{148, 6, NESMirroring::MapperAB},
	{154, 139, NESMirroring::MapperHVAB},
	{216, 6, NESMirroring::MapperHV},

	/* Mapper 010 */
	{238, 6, NESMirroring::MapperHV},
	{251, 251, NESMirroring::Header},
	{264, 277, NESMirroring::MapperHV},
	{281, 6, NESMirroring::Header},
	{291, 6, NESMirroring::MapperHVAB},
	{317, 0, NESMirroring::MapperHV},
	{336, 342, NESMirroring::MapperHVAB},
	{349, 277, NESMirroring::MapperHVAB},
	{357, 366, NESMirroring::MapperHVAB},
	{373, 387, NESMirroring::MapperNamco163},

	/* Mapper 020 */
	{393, 6, NESMirroring::MapperHV},
	{413, 426, NESMirroring::MapperHVAB},
	{433, 426, NESMirroring::MapperHVAB},
	{439, 426, NESMirroring::MapperHVAB},
	{459, 426, NESMirroring::MapperVRC6},
	{465, 426, NESMirroring::MapperHVAB},
	{485, 426, NESMirroring::MapperVRC6},
	{491, 0, NESMirroring::MapperHVAB},
	{504, 514, NESMirroring::MapperHVAB},
	{523, 534, NESMirroring::Header},

	/* Mapper 030 */
	{551, 561, NESMirroring::UNROM512},
	{570, 514, NESMirroring::Header},
	{592, 603, NESMirroring::MapperHV},
	{608, 621, NESMirroring::MapperHV},
	{627, 0, NESMirroring::Header},
	{643, 674, NESMirroring::MapperJY},
	{687, 708, NESMirroring::MapperHV},
	{712, 6, NESMirroring::MapperHV},
	{727, 741, NESMirroring::Header},
	{751, 0, NESMirroring::Header},

	/* Mapper 040 */
	{765, 793, NESMirroring::Header},
	{799, 814, NESMirroring::MapperHV},
	{822, 0, NESMirroring::MapperHV},
	{837, 0, NESMirroring::Header},
	{712, 0, NESMirroring::MapperHV},
	{869, 0, NESMirroring::MapperHV},
	{892, 251, NESMirroring::Header},
	{712, 6, NESMirroring::MapperHV},
	{915, 621, NESMirroring::MapperHV},
	{712, 0, NESMirroring::MapperHV},

	/* Mapper 050 */
	{928, 956, NESMirroring::Header},
	{961, 0, NESMirroring::Unknown},
	{712, 0, NESMirroring::MapperHV},
	{980, 0, NESMirroring::Unknown},
	{1000, 0, NESMirroring::Unknown},
	{1027, 0, NESMirroring::Header},
	{1045, 0, NESMirroring::MapperHV},
	{1082, 0, NESMirroring::MapperHV},
	{1092, 0, NESMirroring::MapperHV},
	{1116, 0, NESMirroring::MapperHV},

	/* Mapper 060 */
	{1146, 0, NESMirroring::Header},
	{1184, 0, NESMirroring::MapperHV},
	{1202, 0, NESMirroring::MapperHV},
	{1227, 793, NESMirroring::MapperHV},
	{1255, 1270, NESMirroring::MapperHV},
	{1277, 603, NESMirroring::MapperHV},
	{1288, 6, NESMirroring::Header},
	{1301, 1311, NESMirroring::MapperHVAB},
	{1319, 1311, NESMirroring::MapperSunsoft4},
	{1329, 1311, NESMirroring::MapperHVAB},

	/* Mapper 070 */
	{1343, 342, NESMirroring::Header},
	{1358, 1384, NESMirroring::Header},
	{1396, 366, NESMirroring::Header},
	{1409, 426, NESMirroring::Header},
	{1414, 1442, NESMirroring::MapperHV},
	{1452, 426, NESMirroring::MapperHV},
	{1457, 387, NESMirroring::Header},
	{1490, 1505, NESMirroring::FourScreen},
	{1511, 0, NESMirroring::Mapper},
	{1548, 1565, NESMirroring::Header},

	/* Mapper 080 */
	{1594, 621, NESMirroring::MapperHV},
	{1607, 793, NESMirroring::Header},
	{1617, 621, NESMirroring::MapperHV},
	{1664, 1664, NESMirroring::MapperHVAB},
	{1674, 0, NESMirroring::Unknown},
	{1683, 426, NESMirroring::MapperHVAB},
	{1688, 366, NESMirroring::Header},
	{1701, 0, NESMirroring::Header},
	{1715, 0, NESMirroring::Header},
	{1734, 1311, NESMirroring::MapperAB},

	/* Mapper 090 */
	{1762, 674, NESMirroring::MapperHVAB},
	{1802, 674, NESMirroring::MapperHV},
	{1835, 366, NESMirroring::Header},
	{1847, 1311, NESMirroring::Header},
	{1876, 6, NESMirroring::Header},
	{1887, 387, NESMirroring::MapperNamcot3425},
	{1899, 342, NESMirroring::Header},
	{1909, 603, NESMirroring::MapperHV},
	{0, 0, NESMirroring::Unknown},
	{1921, 6, NESMirroring::FourScreen},

	/* Mapper 100 */
	{1940, 0, NESMirroring::MapperHV},
	{1967, 1990, NESMirroring::Header},
	{1998, 514, NESMirroring::Unknown},
	{2003, 0, NESMirroring::MapperHV},
	{2036, 0, NESMirroring::Header},
	{2051, 6, NESMirroring::MapperHVAB},
	{2112, 0, NESMirroring::MapperHV},
	{2142, 2155, NESMirroring::Header},
	{2167, 0, NESMirroring::Unknown},
	{2183, 2232, NESMirroring::MapperSachen8259},

	/* Mapper 110 */
	{2239, 2232, NESMirroring::MapperSachen74LS374N},
	{2287, 2322, NESMirroring::MapperGTROM},
	{1715, 0, NESMirroring::MapperHV},
	{2341, 0, NESMirroring::MapperHV},
	{2362, 0, NESMirroring::MapperHV},
	{2395, 2435, NESMirroring::MapperHV},
	{2445, 2472, NESMirroring::MapperHVAB},
	{2479, 2479, NESMirroring::Unknown},
	{2492, 6, NESMirroring::MapperTxSROM},
	{2499, 6, NESMirroring::MapperHV},

	/* Mapper 120 */
	{2505, 0, NESMirroring::Unknown},
	{2542, 2435, NESMirroring::MapperHV},
	{2581, 1311, NESMirroring::Header},
	{2610, 2435, NESMirroring::MapperHV},
	{2639, 0, NESMirroring::Unknown},
	{2664, 2710, NESMirroring::Header},
	{712, 0, NESMirroring::Unknown},
	{2725, 0, NESMirroring::Unknown},
	{2779, 0, NESMirroring::Unknown},
	{2806, 0, NESMirroring::MapperHV},

	/* Mapper 130 */
	{2849, 0, NESMirroring::Unknown},
	{2892, 0, NESMirroring::MapperHV},
	{2926, 708, NESMirroring::Header},
	{2948, 2232, NESMirroring::Header},
	{2960, 0, NESMirroring::MapperHV},
	{2998, 2232, NESMirroring::MapperSachen8259},
	{3038, 2232, NESMirroring::Header},
	{3050, 2232, NESMirroring::MapperSachen8259},
	{3063, 2232, NESMirroring::MapperSachen8259},
	{3076, 2232, NESMirroring::MapperSachen8259},

	/* Mapper 140 */
	{3089, 366, NESMirroring::Header},
	{3125, 2232, NESMirroring::MapperSachen8259},
	{3138, 3169, NESMirroring::Header},
	{3176, 0, NESMirroring::Header},
	{3196, 3230, NESMirroring::Header},
	{3255, 2232, NESMirroring::Header},
	{3280, 0, NESMirroring::Header},
	{3314, 2232, NESMirroring::Header},
	{3326, 3357, NESMirroring::Header},
	{3373, 2232, NESMirroring::Header},

	/* Mapper 150 */
	{3395, 2232, NESMirroring::MapperSachen74LS374N},
	{3417, 426, NESMirroring::FourScreen},
	{3435, 3169, NESMirroring::MapperAB},
	{3465, 342, NESMirroring::MapperHVAB},
	{3495, 387, NESMirroring::MapperAB},
	{3507, 6, NESMirroring::MapperHVAB},
	{3513, 3522, NESMirroring::MapperDIS23C01},
	{3535, 342, NESMirroring::MapperHVAB},
	{3559, 1270, NESMirroring::MapperTxSROM},
	{3573, 342, NESMirroring::MapperHVAB},

	/* Mapper 160 */
	{3599, 674, NESMirroring::MapperHVAB},
	{3658, 6, NESMirroring::MapperHVAB},
	{3694, 1442, NESMirroring::Unknown},
	{3710, 3710, NESMirroring::Header},
	{3718, 3756, NESMirroring::Unknown},
	{3765, 0, NESMirroring::MapperHV},
	{3809, 3827, NESMirroring::Header},
	{3833, 3827, NESMirroring::Header},
	{3851, 3873, NESMirroring::Header},
	{3889, 3889, NESMirroring::Unknown},

	/* Mapper 170 */
	{3896, 0, NESMirroring::Unknown},
	{3911, 3169, NESMirroring::Header},
	{3926, 0, NESMirroring::MapperHV},
	{3944, 3959, NESMirroring::Header},
	{1082, 0, NESMirroring::MapperHV},
	{3968, 3169, NESMirroring::Unknown},
	{3993, 1442, NESMirroring::MapperHVAB},
	{751, 4033, NESMirroring::MapperHV},
	{4051, 4051, NESMirroring::MapperHV},
	{4106, 1442, NESMirroring::MapperHVAB},

	/* Mapper 180 */
	{4165, 4193, NESMirroring::Header},
	{4204, 4193, NESMirroring::Header},
	{4229, 0, NESMirroring::MapperHV},
	{4276, 0, NESMirroring::MapperHVAB},
	{4302, 1311, NESMirroring::Header},
	{4312, 0, NESMirroring::Header},
	{4344, 4354, NESMirroring::Header},
	{4370, 2435, NESMirroring::MapperHV},
	{4400, 342, NESMirroring::MapperHV},
	{4422, 0, NESMirroring::MapperHV},

	/* Mapper 190 */
	{4451, 0, NESMirroring::Header},
	{4468, 0, NESMirroring::MapperHV},
	{4468, 0, NESMirroring::MapperHV},
	{4479, 793, NESMirroring::MapperHV},
	{4468, 0, NESMirroring::MapperHV},
	{4492, 1442, NESMirroring::MapperHV},
	{4521, 0, NESMirroring::MapperHV},
	{4548, 2435, NESMirroring::MapperHV /* not sure */},
	{4571, 0, NESMirroring::MapperHV},
	{4611, 1442, NESMirroring::MapperHVAB},

	/* Mapper 200 */
	{1082, 0, NESMirroring::MapperHV},
	{4657, 0, NESMirroring::Header},
	{4676, 0, NESMirroring::MapperHV},
	{4695, 0, NESMirroring::Header},
	{1082, 0, NESMirroring::Unknown},
	{712, 0, NESMirroring::MapperHV},
	{4713, 6, NESMirroring::Header},
	{4748, 621, NESMirroring::MapperNamcot3425},
	{4765, 0, NESMirroring::MapperHV},
	{4809, 674, NESMirroring::MapperJY},

	/* Mapper 210 */
	{4840, 387, NESMirroring::MapperHVAB},
	{4856, 674, NESMirroring::MapperJY},
	{4898, 0, NESMirroring::MapperHV},
	{4921, 0, NESMirroring::MapperHV},
	{4959, 0, NESMirroring::Unknown},
	{4987, 5013, NESMirroring::MapperHV},
	{5026, 0, NESMirroring::Unknown},
	{5067, 0, NESMirroring::Unknown},
	{5098, 514, NESMirroring::MagicFloor},
	{5110, 2435, NESMirroring::MapperHV},

	/* Mapper 220 */
	{5139, 5167, NESMirroring::Unknown},
	{5178, 793, NESMirroring::MapperHV},
	{5192, 0, NESMirroring::MapperHVAB},
	{5213, 1442, NESMirroring::MapperHVAB},
	{5278, 5292, NESMirroring::Unknown},
	{1082, 0, NESMirroring::MapperHV},
	{1082, 0, NESMirroring::MapperHV},
	{1082, 0, NESMirroring::MapperHV},
	{5299, 5299, NESMirroring::MapperHV},
	{5318, 0, NESMirroring::MapperHV},

	/* Mapper 230 */
	{1082, 0, NESMirroring::MapperHV},
	{1082, 0, NESMirroring::MapperHV},
	{5330, 1384, NESMirroring::Header},
	{1082, 0, NESMirroring::Mapper233},
	{5350, 0, NESMirroring::MapperHV},
	{5368, 0, NESMirroring::Mapper235},
	{5399, 5412, NESMirroring::MapperHV},
	{5420, 0, NESMirroring::MapperHV},
	{5451, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 240 */
	{1082, 0, NESMirroring::Header},
	{5500, 0, NESMirroring::Header},
	{5531, 0, NESMirroring::MapperHV},
	{5542, 2232, NESMirroring::MapperSachen74LS374N},
	{5557, 5567, NESMirroring::Unknown},
	{4468, 0, NESMirroring::MapperHV},
	{5571, 5567, NESMirroring::Header},
	{0, 0, NESMirroring::Unknown},
	{5614, 2435, NESMirroring::MapperHV},
	{5692, 1442, NESMirroring::Unknown},

	/* Mapper 250 */
	{5743, 5762, NESMirroring::MapperHV},
	{5768, 0, NESMirroring::MapperHV},
	{5810, 1442, NESMirroring::MapperHVAB},
	{5833, 1442, NESMirroring::MapperHVAB},
	{5881, 0, NESMirroring::MapperHV},
	{5909, 0, NESMirroring::MapperHV},
	{5942, 0, NESMirroring::Unknown},
	{5959, 0, NESMirroring::Unknown},
	{5972, 0, NESMirroring::Unknown},
	{5982, 0, NESMirroring::Unknown},

	/* Mapper 260 */
	{6009, 0, NESMirroring::Unknown},
	{6033, 0, NESMirroring::Unknown},
	{6060, 2232, NESMirroring::Unknown},
	{6087, 0, NESMirroring::Unknown},
	{6121, 1664, NESMirroring::Unknown},
	{6146, 0, NESMirroring::Unknown},
	{6162, 0, NESMirroring::Unknown},
	{6178, 674, NESMirroring::Unknown},
	{6215, 0, NESMirroring::Unknown},
	{6242, 0, NESMirroring::Unknown},

	/* Mapper 270 */
	{6265, 0, NESMirroring::Unknown},
	{6283, 708, NESMirroring::Unknown},
	{6314, 0, NESMirroring::Unknown},
	{6359, 0, NESMirroring::Unknown},
	{6380, 6406, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 280 */
	{0, 0, NESMirroring::Unknown},
	{6416, 674, NESMirroring::Unknown},
	{6460, 674, NESMirroring::Unknown},
	{6483, 0, NESMirroring::Unknown},
	{1998, 514, NESMirroring::Unknown},
	{6520, 0, NESMirroring::Unknown},
	{6536, 6556, NESMirroring::Unknown},
	{6566, 0, NESMirroring::Unknown},
	{6604, 0, NESMirroring::Unknown},
	{6628, 0, NESMirroring::Unknown},

	/* Mapper 290 */
	{6639, 6663, NESMirroring::Unknown},
	{6669, 2435, NESMirroring::Unknown},
	{6702, 0, NESMirroring::Unknown},
	{6730, 0, NESMirroring::Unknown},
	{6764, 0, NESMirroring::Unknown},
	{6816, 674, NESMirroring::Unknown},
	{6847, 0, NESMirroring::Unknown},
	{6893, 708, NESMirroring::Unknown},
	{6920, 0, NESMirroring::Unknown},
	{6960, 708, NESMirroring::Unknown},

	/* Mapper 300 */
	{6991, 0, NESMirroring::Unknown},
	{7017, 0, NESMirroring::Unknown},
	{7031, 3169, NESMirroring::Unknown},
	{7055, 3169, NESMirroring::Unknown},
	{822, 2710, NESMirroring::Unknown},
	{7089, 3169, NESMirroring::Unknown},
	{7133, 3169, NESMirroring::Unknown},
	{7166, 3169, NESMirroring::Unknown},
	{7191, 0, NESMirroring::Unknown},
	{7231, 2710, NESMirroring::Unknown},

	/* Mapper 310 */
	{7264, 2710, NESMirroring::Unknown},
	{7324, 0, NESMirroring::Unknown},
	{7375, 3169, NESMirroring::Unknown},
	{7398, 0, NESMirroring::Unknown},
	{7427, 0, NESMirroring::Unknown},
	{7441, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{7472, 0, NESMirroring::Unknown},

	/* Mapper 320 */
	{7498, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{7531, 0, NESMirroring::Unknown},
	{7558, 514, NESMirroring::Unknown},
	{7598, 514, NESMirroring::Unknown},
	{7638, 0, NESMirroring::Unknown},
	{7671, 0, NESMirroring::Unknown},
	{7695, 0, NESMirroring::Unknown},
	{7712, 0, NESMirroring::Unknown},
	{7769, 0, NESMirroring::Unknown},

	/* Mapper 330 */
	{7793, 0, NESMirroring::Unknown},
	{7834, 0, NESMirroring::Unknown},
	{7858, 0, NESMirroring::Unknown},
	{7882, 7914, NESMirroring::Unknown},
	{7923, 0, NESMirroring::Unknown},
	{7958, 0, NESMirroring::Unknown},
	{7976, 0, NESMirroring::Unknown},
	{7994, 0, NESMirroring::Unknown},
	{8022, 0, NESMirroring::Unknown},
	{8063, 0, NESMirroring::Unknown},

	/* Mapper 340 */
	{4695, 0, NESMirroring::Unknown},
	{8081, 0, NESMirroring::Unknown},
	{8105, 514, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{8135, 0, NESMirroring::Unknown},
	{8186, 7914, NESMirroring::Unknown},
	{8227, 3169, NESMirroring::Unknown},
	{8250, 3169, NESMirroring::Unknown},
	{8296, 0, NESMirroring::Unknown},
	{8304, 0, NESMirroring::Unknown},

	/* Mapper 350 */
	{8345, 0, NESMirroring::Unknown},
	{8379, 8396, NESMirroring::Unknown},
	{8420, 3169, NESMirroring::Unknown},
	{8453, 0, NESMirroring::Unknown},
	{8485, 0, NESMirroring::Unknown},
	{8504, 0, NESMirroring::Unknown},
	{8523, 674, NESMirroring::Unknown},
	{8547, 741, NESMirroring::Unknown},
	{6460, 674, NESMirroring::Unknown},
	{8571, 0, NESMirroring::Unknown},

	/* Mapper 360 */
	{8609, 741, NESMirroring::Unknown},
	{8634, 674, NESMirroring::Unknown},
	{8667, 674, NESMirroring::Unknown},
	{6460, 674, NESMirroring::Unknown},
	{8699, 674, NESMirroring::Unknown},
	{8719, 6663, NESMirroring::Unknown},
	{8752, 0, NESMirroring::Unknown},
	{8781, 0, NESMirroring::Unknown},
	{8798, 8839, NESMirroring::Unknown},
	{8847, 0, NESMirroring::Unknown},

	/* Mapper 370 */
	{8856, 0, NESMirroring::Unknown},
	{8861, 8901, NESMirroring::Unknown},
	{8908, 0, NESMirroring::Unknown},
	{8939, 0, NESMirroring::Unknown},
	{8971, 0, NESMirroring::Unknown},
	{8998, 0, NESMirroring::Unknown},
	{9025, 674, NESMirroring::Unknown},
	{9045, 674, NESMirroring::Unknown},
	{9077, 0, NESMirroring::Unknown},
	{9106, 0, NESMirroring::Unknown},

	/* Mapper 380 */
	{9129, 0, NESMirroring::Unknown},
	{9137, 0, NESMirroring::Unknown},
	{9143, 0, NESMirroring::Unknown},
	{9151, 674, NESMirroring::Unknown},
	{9174, 0, NESMirroring::Unknown},
	{9194, 793, NESMirroring::Unknown},
	{9205, 674, NESMirroring::Unknown},
	{9215, 674, NESMirroring::Unknown},
	{9237, 674, NESMirroring::Unknown},
	{9259, 814, NESMirroring::Unknown},

	/* Mapper 390 */
	{9284, 5412, NESMirroring::Unknown},
	{9297, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{9318, 5412, NESMirroring::Unknown},
	{9343, 5412, NESMirroring::Unknown},
	{9384, 0, NESMirroring::Unknown},
	{9420, 674, NESMirroring::Unknown},
	{9448, 674, NESMirroring::Unknown},
	{9458, 514, NESMirroring::Unknown},

	/* Mapper 400 */
	{9470, 9486, NESMirroring::Unknown},
	{9495, 0, NESMirroring::Unknown},
	{9511, 0, NESMirroring::Unknown},
	{9528, 0, NESMirroring::Unknown},
	{9534, 674, NESMirroring::Unknown},
	{9564, 9593, NESMirroring::Unknown},
	{9597, 514, NESMirroring::Unknown},
	{9611, 0, NESMirroring::Unknown},
	{9649, 426, NESMirroring::Unknown},
	{9702, 9486, NESMirroring::Unknown},

	/* Mapper 410 */
	{9711, 674, NESMirroring::Unknown},
	{9743, 0, NESMirroring::Unknown},
	{9778, 9825, NESMirroring::Unknown},
	{9839, 514, NESMirroring::Unknown},
	{9862, 0, NESMirroring::Unknown},
	{9885, 0, NESMirroring::Unknown},
	{9938, 0, NESMirroring::Unknown},
	{9955, 10000, NESMirroring::Unknown},
	{10012, 0, NESMirroring::Unknown},
	{10038, 10057, NESMirroring::Unknown},

	/* Mapper 420 */
	{10064, 10072, NESMirroring::Unknown},
	{10080, 674, NESMirroring::Unknown},
	{10090, 0, NESMirroring::Unknown},
	{10127, 10165, NESMirroring::Unknown},
	{10174, 10165, NESMirroring::Unknown},
	{10213, 10247, NESMirroring::Unknown},
	{10257, 0, NESMirroring::Unknown},
	{10311, 0, NESMirroring::Unknown},
	{10366, 0, NESMirroring::Unknown},
	{10384, 10424, NESMirroring::Unknown},

	/* Mapper 430 */
	{10433, 0, NESMirroring::Unknown},
	{10470, 5412, NESMirroring::Unknown},
	{10485, 5412, NESMirroring::Unknown},
	{10511, 0, NESMirroring::Unknown},
	{10551, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 440 */
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{10557, 0, NESMirroring::Unknown},
	{10575, 0, NESMirroring::Unknown},
	{10611, 0, NESMirroring::Unknown},
	{10646, 10659, NESMirroring::Unknown},
	{10668, 0, NESMirroring::Unknown},
	{10691, 0, NESMirroring::Unknown},
	{10716, 0, NESMirroring::Unknown},

	/* Mapper 450 */
	{10733, 674, NESMirroring::Unknown},
	{10760, 0, NESMirroring::Unknown},
	{10776, 0, NESMirroring::Unknown},
	{10794, 5412, NESMirroring::Unknown},
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
	{10807, 0, NESMirroring::Unknown},
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
	{10819, 2232, NESMirroring::Unknown},
	{10839, 2232, NESMirroring::Unknown},
	{10875, 3827, NESMirroring::Unknown},
	{10889, 0, NESMirroring::Unknown},
	{10906, 10932, NESMirroring::Unknown},
	{10940, 0, NESMirroring::Unknown},
	{10963, 3827, NESMirroring::Unknown},
	{10979, 0, NESMirroring::Unknown},

	/* Mapper 520 */
	{10991, 0, NESMirroring::Unknown},
	{11029, 0, NESMirroring::Unknown},
	{11040, 2710, NESMirroring::Unknown},
	{11074, 5292, NESMirroring::Unknown},
	{11120, 0, NESMirroring::Unknown},
	{11156, 3169, NESMirroring::Unknown},
	{11182, 0, NESMirroring::Unknown},
	{11221, 0, NESMirroring::Unknown},
	{11262, 0, NESMirroring::Unknown},
	{11301, 0, NESMirroring::Unknown},

	/* Mapper 530 */
	{11346, 0, NESMirroring::Unknown},
	{11390, 6663, NESMirroring::Unknown},
	{11406, 0, NESMirroring::Unknown},
	{11420, 2232, NESMirroring::Unknown},
	{11432, 0, NESMirroring::Unknown},
	{11474, 2710, NESMirroring::Unknown},
	{11511, 1442, NESMirroring::Unknown},
	{11511, 1442, NESMirroring::Unknown},
	{11554, 0, NESMirroring::Unknown},
	{11566, 0, NESMirroring::Unknown},

	/* Mapper 540 */
	{11594, 0, NESMirroring::Unknown},
	{11635, 0, NESMirroring::Unknown},
	{11664, 0, NESMirroring::Unknown},
	{11693, 0, NESMirroring::Unknown},
	{11732, 1442, NESMirroring::Unknown},
	{11748, 0, NESMirroring::Unknown},
	{11763, 0, NESMirroring::Unknown},
	{11797, 426, NESMirroring::Unknown},
	{11823, 11830, NESMirroring::Unknown},
	{11842, 3169, NESMirroring::Unknown},

	/* Mapper 550 */
	{11862, 674, NESMirroring::Unknown},
	{11872, 5292, NESMirroring::Unknown},
	{11911, 621, NESMirroring::Unknown},
	{11956, 2232, NESMirroring::Unknown},
	{11968, 3169, NESMirroring::Unknown},
	{11983, 6, NESMirroring::Unknown},
	{12033, 674, NESMirroring::Unknown},
	{12050, 3169, NESMirroring::Unknown},
	{12107, 12116, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 560 */
	{0, 0, NESMirroring::Unknown},
	{12127, 139, NESMirroring::Unknown},
	{12170, 0, NESMirroring::Unknown},
};
