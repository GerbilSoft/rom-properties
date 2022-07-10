/** NES Mappers (generated from NESMappers_data.txt) **/

#include <stdint.h>

static const char8_t NESMappers_strtbl[] =
	U8("\x00") U8("NROM") U8("\x00") U8("Nintendo") U8("\x00") U8("SxROM")
	U8(" (MMC1)") U8("\x00") U8("UxROM") U8("\x00") U8("CNROM") U8("\x00")
	U8("TxROM (MMC3), HKROM (MMC6)") U8("\x00") U8("ExROM (MMC5)") U8("\x00")
	U8("Front Fareast (Game Doctor) Magic Card 1M/2M RAM cartridge") U8("\x00")
	U8("Bung/FFE") U8("\x00") U8("AxROM") U8("\x00") U8("Front Fareast (")
	U8("Game Doctor) GxROM (clone of 006, submapper 4)") U8("\x00") U8("P")
	U8("xROM, PEEOROM (MMC2)") U8("\x00") U8("FxROM (MMC4)") U8("\x00") U8("C")
	U8("olor Dreams") U8("\x00") U8("MMC3 variant") U8("\x00") U8("FFE") U8("\x00")
	U8("NES-CPROM") U8("\x00") U8("SL-1632 (MMC3/VRC2 clone)") U8("\x00") U8("K")
	U8("-1029 (multicart)") U8("\x00") U8("FCG-x") U8("\x00") U8("Bandai")
	U8("\x00") U8("FFE #17") U8("\x00") U8("SS 88006") U8("\x00") U8("Ja")
	U8("leco") U8("\x00") U8("Namco 129/163") U8("\x00") U8("Namco") U8("\x00")
	U8("Famicom Disk System") U8("\x00") U8("VRC4a, VRC4c") U8("\x00") U8("K")
	U8("onami") U8("\x00") U8("VRC2a") U8("\x00") U8("VRC4e, VRC4f, VRC2")
	U8("b") U8("\x00") U8("VRC6a") U8("\x00") U8("VRC4b, VRC4d, VRC2c") U8("\x00")
	U8("VRC6b") U8("\x00") U8("VRC4 variant") U8("\x00") U8("Action 53") U8("\x00")
	U8("Homebrew") U8("\x00") U8("RET-CUFROM") U8("\x00") U8("Sealie Com")
	U8("puting") U8("\x00") U8("UNROM 512") U8("\x00") U8("RetroUSB") U8("\x00")
	U8("NSF Music Compilation") U8("\x00") U8("Irem G-101") U8("\x00") U8("I")
	U8("rem") U8("\x00") U8("Taito TC0190") U8("\x00") U8("Taito") U8("\x00")
	U8("BNROM, NINA-001") U8("\x00") U8("J.Y. Company ASIC (8 KiB WRAM)") U8("\x00")
	U8("J.Y. Company") U8("\x00") U8("TXC PCB 01-22000-400") U8("\x00") U8("T")
	U8("XC") U8("\x00") U8("MMC3 multicart") U8("\x00") U8("GNROM varian")
	U8("t") U8("\x00") U8("Bit Corp.") U8("\x00") U8("BNROM variant") U8("\x00")
	U8("NTDEC 2722 (FDS conversion)") U8("\x00") U8("NTDEC") U8("\x00") U8("C")
	U8("altron 6-in-1") U8("\x00") U8("Caltron") U8("\x00") U8("FDS conv")
	U8("ersion") U8("\x00") U8("TONY-I, YS-612 (FDS conversion)") U8("\x00")
	U8("MMC3 multicart (GA23C)") U8("\x00") U8("Rumble Station 15-in-1") U8("\x00")
	U8("Taito TC0690") U8("\x00") U8("PCB 761214 (FDS conversion)") U8("\x00")
	U8("N-32") U8("\x00") U8("11-in-1 Ball Games") U8("\x00") U8("Superv")
	U8("ision 16-in-1") U8("\x00") U8("Novel Diamond 9999999-in-1") U8("\x00")
	U8("BTL-MARIO1-MALEE2") U8("\x00") U8("KS202 (unlicensed SMB3 reprod")
	U8("uction)") U8("\x00") U8("Multicart") U8("\x00") U8("(C)NROM-base")
	U8("d multicart") U8("\x00") U8("BMC-T3H53/BMC-D1038 multicart") U8("\x00")
	U8("Reset-based NROM-128 4-in-1 multicart") U8("\x00") U8("20-in-1 m")
	U8("ulticart") U8("\x00") U8("Super 700-in-1 multicart") U8("\x00") U8("P")
	U8("owerful 250-in-1 multicart") U8("\x00") U8("Tengen RAMBO-1") U8("\x00")
	U8("Tengen") U8("\x00") U8("Irem H3001") U8("\x00") U8("GxROM, MHROM")
	U8("\x00") U8("Sunsoft-3") U8("\x00") U8("Sunsoft") U8("\x00") U8("S")
	U8("unsoft-4") U8("\x00") U8("Sunsoft FME-7") U8("\x00") U8("Family ")
	U8("Trainer") U8("\x00") U8("Codemasters (UNROM clone)") U8("\x00") U8("C")
	U8("odemasters") U8("\x00") U8("Jaleco JF-17") U8("\x00") U8("VRC3") U8("\x00")
	U8("43-393/860908C (MMC3 clone)") U8("\x00") U8("W") U8("\xc3\xa0") U8("i")
	U8("x") U8("\xc4\xab") U8("ng") U8("\x00") U8("VRC1") U8("\x00") U8("N")
	U8("AMCOT-3446 (Namcot 108 variant)") U8("\x00") U8("Napoleon Senki") U8("\x00")
	U8("Lenar") U8("\x00") U8("Holy Diver; Uchuusen - Cosmo Carrier") U8("\x00")
	U8("NINA-03, NINA-06") U8("\x00") U8("American Video Entertainment") U8("\x00")
	U8("Taito X1-005") U8("\x00") U8("Super Gun") U8("\x00") U8("Taito X")
	U8("1-017 (incorrect PRG ROM bank ordering)") U8("\x00") U8("Cony/Yo")
	U8("ko") U8("\x00") U8("PC-SMB2J") U8("\x00") U8("VRC7") U8("\x00") U8("J")
	U8("aleco JF-13") U8("\x00") U8("CNROM variant") U8("\x00") U8("Namc")
	U8("ot 118 variant") U8("\x00") U8("Sunsoft-2 (Sunsoft-3 board)") U8("\x00")
	U8("J.Y. Company (simple nametable control)") U8("\x00") U8("J.Y. Co")
	U8("mpany (Super Fighter III)") U8("\x00") U8("Moero!! Pro") U8("\x00")
	U8("Sunsoft-2 (Sunsoft-3R board)") U8("\x00") U8("HVC-UN1ROM") U8("\x00")
	U8("NAMCOT-3425") U8("\x00") U8("Oeka Kids") U8("\x00") U8("Irem TAM")
	U8("-S1") U8("\x00") U8("CNROM (Vs. System)") U8("\x00") U8("MMC3 va")
	U8("riant (hacked ROMs)") U8("\x00") U8("Jaleco JF-10 (misdump)") U8("\x00")
	U8("Jaleceo") U8("\x00") U8("Drip") U8("\x00") U8("Doki Doki Panic (")
	U8("FDS conversion)") U8("\x00") U8("PEGASUS 5 IN 1") U8("\x00") U8("N")
	U8("ES-EVENT (MMC1 variant) (Nintendo World Championships 1990)") U8("\x00")
	U8("Super Mario Bros. 3 (bootleg)") U8("\x00") U8("Magic Dragon") U8("\x00")
	U8("Magicseries") U8("\x00") U8("FDS conversions") U8("\x00") U8("Th")
	U8("e Great Wall (Sachen 8259D) (duplicate of 137)") U8("\x00") U8("S")
	U8("achen") U8("\x00") U8("Honey Peach (Sachen SA-020A) (duplicate o")
	U8("f 243)") U8("\x00") U8("Cheapocabra GTROM 512k flash board") U8("\x00")
	U8("Membler Industries") U8("\x00") U8("NINA-03/06 multicart") U8("\x00")
	U8("MMC3 clone (scrambled registers)") U8("\x00") U8("K") U8("\xc7\x8e")
	U8("sh") U8("\xc3\xa8") U8("ng SFC-02B/-03/-004 (MMC3 clone)") U8("\x00")
	U8("K") U8("\xc7\x8e") U8("sh") U8("\xc3\xa8") U8("ng") U8("\x00") U8("S")
	U8("OMARI-P (Huang-1/Huang-2)") U8("\x00") U8("Gouder") U8("\x00") U8("F")
	U8("uture Media") U8("\x00") U8("TxSROM") U8("\x00") U8("TQROM") U8("\x00")
	U8("Tobidase Daisakusen (FDS conversion)") U8("\x00") U8("K") U8("\xc7")
	U8("\x8e") U8("sh") U8("\xc3\xa8") U8("ng A9711 and A9713 (MMC3 clon")
	U8("e)") U8("\x00") U8("Sunsoft-1 (duplicate of 184)") U8("\x00") U8("K")
	U8("\xc7\x8e") U8("sh") U8("\xc3\xa8") U8("ng H2288 (MMC3 clone)") U8("\x00")
	U8("Super Game Mega Type III") U8("\x00") U8("Monty no Doki Doki Dai")
	U8("sass") U8("\xc5\x8d") U8(" (FDS conversion)") U8("\x00") U8("Whi")
	U8("rlwind Manu") U8("\x00") U8("Double Dragon - The Revenge (Japan)")
	U8(" (pirated version)") U8("\x00") U8("Super HiK 4-in-1 multicart") U8("\x00")
	U8("(C)NROM-based multicart (duplicate of 058)") U8("\x00") U8("7-in")
	U8("-1 (NS03) multicart (duplicate of 331)") U8("\x00") U8("MMC3 mul")
	U8("ticart (duplicate of 205)") U8("\x00") U8("TXC 05-00002-010 ASIC")
	U8("\x00") U8("Jovial Race") U8("\x00") U8("T4A54A, WX-KB4K, BS-5652")
	U8(" (MMC3 clone)") U8("\x00") U8("Sachen 8259A variant (duplicate o")
	U8("f 141)") U8("\x00") U8("Sachen 3011") U8("\x00") U8("Sachen 8259")
	U8("D") U8("\x00") U8("Sachen 8259B") U8("\x00") U8("Sachen 8259C") U8("\x00")
	U8("Jaleco JF-11, JF-14 (GNROM variant)") U8("\x00") U8("Sachen 8259")
	U8("A") U8("\x00") U8("Kaiser KS202 (FDS conversions)") U8("\x00") U8("K")
	U8("aiser") U8("\x00") U8("Copy-protected NROM") U8("\x00") U8("Deat")
	U8("h Race (Color Dreams variant)") U8("\x00") U8("American Game Car")
	U8("tridges") U8("\x00") U8("Sidewinder (CNROM clone)") U8("\x00") U8("G")
	U8("alactic Crusader (NINA-06 clone)") U8("\x00") U8("Sachen 3018") U8("\x00")
	U8("Sachen SA-008-A, Tengen 800008") U8("\x00") U8("Sachen / Tengen") U8("\x00")
	U8("SA-0036 (CNROM clone)") U8("\x00") U8("Sachen SA-015, SA-630") U8("\x00")
	U8("VRC1 (Vs. System)") U8("\x00") U8("Kaiser KS202 (FDS conversion)")
	U8("\x00") U8("Bandai FCG: LZ93D50 with SRAM") U8("\x00") U8("NAMCOT")
	U8("-3453") U8("\x00") U8("MMC1A") U8("\x00") U8("DIS23C01") U8("\x00")
	U8("Daou Infosys") U8("\x00") U8("Datach Joint ROM System") U8("\x00")
	U8("Tengen 800037") U8("\x00") U8("Bandai LZ93D50 with 24C01") U8("\x00")
	U8("J.Y. Company (simple nametable control) (duplicate of 090)") U8("\x00")
	U8("Hanjuku Hero (MMC1) (should be 001)") U8("\x00") U8("W") U8("\xc3")
	U8("\xa0") U8("ix") U8("\xc4\xab") U8("ng FS304") U8("\x00") U8("Nan")
	U8("jing") U8("\x00") U8("PEC-9588 Pyramid Educational Computer") U8("\x00")
	U8("D") U8("\xc5\x8d") U8("ngd") U8("\xc3\xa1\x00") U8("Fire Emblem ")
	U8("(unlicensed) (MMC2+MMC3 hybrid)") U8("\x00") U8("Subor (variant ")
	U8("1)") U8("\x00") U8("Subor") U8("\x00") U8("Subor (variant 2)") U8("\x00")
	U8("Racermate Challenge 2") U8("\x00") U8("Racermate, Inc.") U8("\x00")
	U8("Yuxing") U8("\x00") U8("Shiko Game Syu") U8("\x00") U8("Kaiser K")
	U8("S-7058") U8("\x00") U8("Super Mega P-4040") U8("\x00") U8("Idea-")
	U8("Tek ET-xx") U8("\x00") U8("Idea-Tek") U8("\x00") U8("Kaiser 15-i")
	U8("n-1 multicart") U8("\x00") U8("W") U8("\xc3\xa0") U8("ix") U8("\xc4")
	U8("\xab") U8("ng multicart (8025) (MMC3 clone)") U8("\x00") U8("H") U8("\xc3")
	U8("\xa9") U8("ngg") U8("\xc3\xa9") U8(" Di") U8("\xc3\xa0") U8("nz") U8("\xc7")
	U8("\x90\x00") U8("W") U8("\xc3\xa0") U8("ix") U8("\xc4\xab") U8("ng")
	U8(" / Nanjing / Jncota / Henge Dianzi / GameStar") U8("\x00") U8("W")
	U8("\xc3\xa0") U8("ix") U8("\xc4\xab") U8("ng multicart (8025) (MMC3")
	U8(" clone) (duplicate of 176)") U8("\x00") U8("Crazy Climber (UNROM")
	U8(" clone)") U8("\x00") U8("Nichibutsu") U8("\x00") U8("Seicross v2")
	U8(" (FCEUX hack)") U8("\x00") U8("MMC3 clone (scrambled registers) ")
	U8("(same as 114)") U8("\x00") U8("Suikan Pipe (VRC4e clone)") U8("\x00")
	U8("Sunsoft-1") U8("\x00") U8("CNROM with weak copy protection") U8("\x00")
	U8("Study Box") U8("\x00") U8("Fukutake Shoten") U8("\x00") U8("K") U8("\xc7")
	U8("\x8e") U8("sh") U8("\xc3\xa8") U8("ng A98402 (MMC3 clone)") U8("\x00")
	U8("Bandai Karaoke Studio") U8("\x00") U8("Thunder Warrior (MMC3 clo")
	U8("ne)") U8("\x00") U8("Magic Kid GooGoo") U8("\x00") U8("MMC3 clon")
	U8("e") U8("\x00") U8("NTDEC TC-112") U8("\x00") U8("W") U8("\xc3\xa0")
	U8("ix") U8("\xc4\xab") U8("ng FS303 (MMC3 clone)") U8("\x00") U8("M")
	U8("ario bootleg (MMC3 clone)") U8("\x00") U8("K") U8("\xc7\x8e") U8("s")
	U8("h") U8("\xc3\xa8") U8("ng (MMC3 clone)") U8("\x00") U8("T") U8("\xc5")
	U8("\xab") U8("nsh") U8("\xc3\xad") U8(" Ti") U8("\xc4\x81") U8("nd") U8("\xc3")
	U8("\xac") U8(" - S") U8("\xc4\x81") U8("ngu") U8("\xc3\xb3") U8(" W")
	U8("\xc3\xa0") U8("izhu") U8("\xc3\xa0") U8("n") U8("\x00") U8("W") U8("\xc3")
	U8("\xa0") U8("ix") U8("\xc4\xab") U8("ng (clone of either Mapper 00")
	U8("4 or 176)") U8("\x00") U8("NROM-256 multicart") U8("\x00") U8("1")
	U8("50-in-1 multicart") U8("\x00") U8("35-in-1 multicart") U8("\x00") U8("D")
	U8("xROM (Tengen MIMIC-1, Namcot 118)") U8("\x00") U8("Fudou Myouou ")
	U8("Den") U8("\x00") U8("Street Fighter IV (unlicensed) (MMC3 clone)")
	U8("\x00") U8("J.Y. Company (MMC2/MMC4 clone)") U8("\x00") U8("Namco")
	U8("t 175, 340") U8("\x00") U8("J.Y. Company (extended nametable con")
	U8("trol)") U8("\x00") U8("BMC Super HiK 300-in-1") U8("\x00") U8("(")
	U8("C)NROM-based multicart (same as 058)") U8("\x00") U8("Super Gun ")
	U8("20-in-1 multicart") U8("\x00") U8("Sugar Softec (MMC3 clone)") U8("\x00")
	U8("Sugar Softec") U8("\x00") U8("Bonza, Magic Jewelry II (Russian; ")
	U8("Dendy)") U8("\x00") U8("500-in-1 / 2000-in-1 multicart") U8("\x00")
	U8("Magic Floor") U8("\x00") U8("K") U8("\xc7\x8e") U8("sh") U8("\xc3")
	U8("\xa8") U8("ng A9461 (MMC3 clone)") U8("\x00") U8("Summer Carniva")
	U8("l '92 - Recca") U8("\x00") U8("Naxat Soft") U8("\x00") U8("NTDEC")
	U8(" N625092") U8("\x00") U8("CTC-31 (VRC2 + 74xx)") U8("\x00") U8("W")
	U8("\xc3\xa0") U8("ix") U8("\xc4\xab") U8("ng (clone of either Mappe")
	U8("r 004 or 176) (duplicate of 199)") U8("\x00") U8("Jncota KT-008") U8("\x00")
	U8("Jncota") U8("\x00") U8("Active Enterprises") U8("\x00") U8("BMC ")
	U8("31-IN-1") U8("\x00") U8("Codemasters Quattro") U8("\x00") U8("Ma")
	U8("xi 15 multicart") U8("\x00") U8("Golden Game 150-in-1 multicart") U8("\x00")
	U8("Realtec 8155") U8("\x00") U8("Realtec") U8("\x00") U8("Teletubbi")
	U8("es 420-in-1 multicart") U8("\x00") U8("Contra Fighter (G.I. Joe ")
	U8("w/ SF2 characters hack)") U8("\x00") U8("BNROM variant (similar ")
	U8("to 034)") U8("\x00") U8("Unlicensed") U8("\x00") U8("Sachen SA-0")
	U8("20A") U8("\x00") U8("Decathlon") U8("\x00") U8("C&E") U8("\x00") U8("F")
	U8("\xc4\x93") U8("ngsh") U8("\xc3\xa9") U8("nb") U8("\xc7\x8e") U8("n")
	U8("g: F") U8("\xc3\xba") U8("m") U8("\xc3\xb3") U8(" S") U8("\xc4\x81")
	U8("n T") U8("\xc3\xa0") U8("iz") U8("\xc7\x90") U8(" (C&E)") U8("\x00")
	U8("K") U8("\xc7\x8e") U8("sh") U8("\xc3\xa8") U8("ng SFC-02B/-03/-0")
	U8("04 (MMC3 clone) (incorrect assignment; should be 115)") U8("\x00")
	U8("W") U8("\xc3\xa0") U8("ix") U8("\xc4\xab") U8("ng MMC3 TKROM clo")
	U8("ne with address scrambling") U8("\x00") U8("Nitra (MMC3 clone)") U8("\x00")
	U8("Nitra") U8("\x00") U8("MMC3 multicart (GA23C) (duplicate of 045)")
	U8("\x00") U8("W") U8("\xc3\xa0") U8("ix") U8("\xc4\xab") U8("ng - S")
	U8("angokushi") U8("\x00") U8("Dragon Ball Z: Ky") U8("\xc5\x8d") U8("s")
	U8("h") U8("\xc5\xab") U8("! Saiya-jin (VRC4 clone)") U8("\x00") U8("P")
	U8("ikachu Y2K of crypted ROMs") U8("\x00") U8("110-in-1 multicart (")
	U8("same as 225)") U8("\x00") U8("OneBus Famiclone") U8("\x00") U8("U")
	U8("NIF PEC-586") U8("\x00") U8("UNIF 158B") U8("\x00") U8("UNIF F-1")
	U8("5 (MMC3 multicart)") U8("\x00") U8("HP10xx/HP20xx multicart") U8("\x00")
	U8("200-in-1 Elfland multicart") U8("\x00") U8("Street Heroes (MMC3 ")
	U8("clone)") U8("\x00") U8("King of Fighters '97 (MMC3 clone)") U8("\x00")
	U8("Cony/Yoko Fighting Games") U8("\x00") U8("T-262 multicart") U8("\x00")
	U8("City Fighter IV") U8("\x00") U8("8-in-1 JY-119 multicart (MMC3 c")
	U8("lone)") U8("\x00") U8("SMD132/SMD133 (MMC3 clone)") U8("\x00") U8("M")
	U8("ulticart (MMC3 clone)") U8("\x00") U8("Game Prince RS-16") U8("\x00")
	U8("TXC 4-in-1 multicart (MGC-026)") U8("\x00") U8("Akumaj") U8("\xc5")
	U8("\x8d") U8(" Special: Boku Dracula-kun (bootleg)") U8("\x00") U8("G")
	U8("remlins 2 (bootleg)") U8("\x00") U8("Cartridge Story multicart") U8("\x00")
	U8("RCM Group") U8("\x00") U8("J.Y. Company Super HiK 3/4/5-in-1 mul")
	U8("ticart") U8("\x00") U8("J.Y. Company multicart") U8("\x00") U8("B")
	U8("lock Family 6-in-1/7-in-1 multicart") U8("\x00") U8("A65AS multi")
	U8("cart") U8("\x00") U8("Benshieng multicart") U8("\x00") U8("Bensh")
	U8("ieng") U8("\x00") U8("4-in-1 multicart (411120-C, 811120-C)") U8("\x00")
	U8("GKCX1 21-in-1 multicart") U8("\x00") U8("BMC-60311C") U8("\x00") U8("A")
	U8("sder 20-in-1 multicart") U8("\x00") U8("Asder") U8("\x00") U8("K")
	U8("\xc7\x8e") U8("sh") U8("\xc3\xa8") U8("ng 2-in-1 multicart (MK6)")
	U8("\x00") U8("Dragon Fighter (unlicensed)") U8("\x00") U8("NewStar ")
	U8("12-in-1/76-in-1 multicart") U8("\x00") U8("T4A54A, WX-KB4K, BS-5")
	U8("652 (MMC3 clone) (same as 134)") U8("\x00") U8("J.Y. Company 13-")
	U8("in-1 multicart") U8("\x00") U8("FC Pocket RS-20 / dreamGEAR My A")
	U8("rcade Gamer V") U8("\x00") U8("TXC 01-22110-000 multicart") U8("\x00")
	U8("Lethal Weapon (unlicensed) (VRC4 clone)") U8("\x00") U8("TXC 6-i")
	U8("n-1 multicart (MGC-023)") U8("\x00") U8("Golden 190-in-1 multica")
	U8("rt") U8("\x00") U8("GG1 multicart") U8("\x00") U8("Gyruss (FDS c")
	U8("onversion)") U8("\x00") U8("Almana no Kiseki (FDS conversion)") U8("\x00")
	U8("Dracula II: Noroi no F") U8("\xc5\xab") U8("in (FDS conversion)") U8("\x00")
	U8("Exciting Basket (FDS conversion)") U8("\x00") U8("Metroid (FDS c")
	U8("onversion)") U8("\x00") U8("Batman (Sunsoft) (bootleg) (VRC2 clo")
	U8("ne)") U8("\x00") U8("Ai Senshi Nicol (FDS conversion)") U8("\x00")
	U8("Monty no Doki Doki Daisass") U8("\xc5\x8d") U8(" (FDS conversion")
	U8(") (same as 125)") U8("\x00") U8("Super Mario Bros. 2 pirate cart")
	U8(" (duplicate of 043)") U8("\x00") U8("Highway Star (bootleg)") U8("\x00")
	U8("Reset-based multicart (MMC3)") U8("\x00") U8("Y2K multicart") U8("\x00")
	U8("820732C- or 830134C- multicart") U8("\x00") U8("HP-898F / KD-7/9")
	U8("-E boards") U8("\x00") U8("Super HiK 6-in-1 A-030 multicart") U8("\x00")
	U8("35-in-1 (K-3033) multicart") U8("\x00") U8("Farid's homebrew 8-i")
	U8("n-1 SLROM multicart") U8("\x00") U8("Farid's homebrew 8-in-1 UNR")
	U8("OM multicart") U8("\x00") U8("Super Mali Splash Bomb (bootleg)") U8("\x00")
	U8("Contra/Gryzor (bootleg)") U8("\x00") U8("6-in-1 multicart") U8("\x00")
	U8("Test Ver. 1.01 Dlya Proverki TV Pristavok test cartridge") U8("\x00")
	U8("Education Computer 2000") U8("\x00") U8("Sangokushi II: Ha") U8("\xc5")
	U8("\x8d") U8(" no Tairiku (bootleg)") U8("\x00") U8("7-in-1 (NS03) ")
	U8("multicart") U8("\x00") U8("Super 40-in-1 multicart") U8("\x00") U8("N")
	U8("ew Star Super 8-in-1 multicart") U8("\x00") U8("New Star") U8("\x00")
	U8("5/20-in-1 1993 Copyright multicart") U8("\x00") U8("10-in-1 mult")
	U8("icart") U8("\x00") U8("11-in-1 multicart") U8("\x00") U8("12-in-")
	U8("1 Game Card multicart") U8("\x00") U8("16-in-1, 200/300/600/1000")
	U8("-in-1 multicart") U8("\x00") U8("21-in-1 multicart") U8("\x00") U8("S")
	U8("imple 4-in-1 multicart") U8("\x00") U8("COOLGIRL multicart (Home")
	U8("brew)") U8("\x00") U8("Kuai Da Jin Ka Zhong Ji Tiao Zhan 3-in-1 ")
	U8("multicart") U8("\x00") U8("New Star 6-in-1 Game Cartridge multic")
	U8("art") U8("\x00") U8("Zanac (FDS conversion)") U8("\x00") U8("Yum")
	U8("e Koujou: Doki Doki Panic (FDS conversion)") U8("\x00") U8("8301")
	U8("18C") U8("\x00") U8("1994 Super HIK 14-in-1 (G-136) multicart") U8("\x00")
	U8("Super 15-in-1 Game Card multicart") U8("\x00") U8("9-in-1 multic")
	U8("art") U8("\x00") U8("J.Y. Company / Techline") U8("\x00") U8("Ka")
	U8("iser 4-in-1 multicart (KS106C)") U8("\x00") U8("92 Super Mario F")
	U8("amily multicart") U8("\x00") U8("250-in-1 multicart") U8("\x00\xe9")
	U8("\xbb\x83\xe4\xbf\xa1\xe7\xb6\xad") U8(" 3D-BLOCK") U8("\x00") U8("7")
	U8("-in-1 Rockman (JY-208)") U8("\x00") U8("4-in-1 (4602) multicart") U8("\x00")
	U8("SB-5013 / GCL8050 / 841242C multicart") U8("\x00") U8("31-in-1 (")
	U8("3150) multicart") U8("\x00") U8("YY841101C multicart (MMC3 clone")
	U8(")") U8("\x00") U8("830506C multicart (VRC4f clone)") U8("\x00") U8("J")
	U8("Y830832C multicart") U8("\x00") U8("Asder PC-95 educational comp")
	U8("uter") U8("\x00") U8("GN-45 multicart (MMC3 clone)") U8("\x00") U8("7")
	U8("-in-1 multicart") U8("\x00") U8("Super Mario Bros. 2 (J) (FDS co")
	U8("nversion)") U8("\x00") U8("YUNG-08") U8("\x00") U8("N49C-300") U8("\x00")
	U8("F600") U8("\x00") U8("Spanish PEC-586 home computer cartridge") U8("\x00")
	U8("Dongda") U8("\x00") U8("Rockman 1-6 (SFC-12) multicart") U8("\x00")
	U8("Super 4-in-1 (SFC-13) multicart") U8("\x00") U8("Reset-based MMC")
	U8("1 multicart") U8("\x00") U8("135-in-1 (U)NROM multicart") U8("\x00")
	U8("YY841155C multicart") U8("\x00") U8("1998 Super Game 8-in-1 (JY-")
	U8("111)") U8("\x00") U8("8-in-1 AxROM/UNROM multicart") U8("\x00") U8("3")
	U8("5-in-1 NROM multicart") U8("\x00") U8("970630C") U8("\x00") U8("K")
	U8("N-42") U8("\x00") U8("830928C") U8("\x00") U8("YY840708C (MMC3 c")
	U8("lone)") U8("\x00") U8("L1A16 (VRC4e clone)") U8("\x00") U8("NTDE")
	U8("C 2779") U8("\x00") U8("YY860729C") U8("\x00") U8("YY850735C / Y")
	U8("Y850817C") U8("\x00") U8("YY841145C / YY850835C") U8("\x00") U8("C")
	U8("altron 9-in-1 multicart") U8("\x00") U8("Realtec 8031") U8("\x00")
	U8("NC7000M (MMC3 clone)") U8("\x00") U8("Realtec HSK007 multicart") U8("\x00")
	U8("Realtec 8210 multicart (MMC3-compatible)") U8("\x00") U8("YY8504")
	U8("37C / Realtec GN-51 multicart") U8("\x00") U8("1996 Soccer 6-in-")
	U8("1 (JY-082)") U8("\x00") U8("YY840820C") U8("\x00") U8("Star Vers")
	U8("us") U8("\x00") U8("8-BIT XMAS 2017") U8("\x00") U8("retroUSB") U8("\x00")
	U8("KC885 multicart") U8("\x00") U8("J-2282 multicart") U8("\x00") U8("8")
	U8("9433") U8("\x00") U8("JY012005 MMC1-based multicart") U8("\x00") U8("G")
	U8("ame designed for UMC UM6578") U8("\x00") U8("UMC") U8("\x00") U8("H")
	U8("aradius Zero") U8("\x00") U8("Win, Lose, or Draw Plug-n-Play (VT")
	U8("03)") U8("\x00") U8("Konami Collector's Series Advance Arcade Pl")
	U8("ug & Play") U8("\x00") U8("DPCMcart") U8("\x00") U8("Super 8-in-")
	U8("1 multicart (JY-302)") U8("\x00") U8("A88S-1 multicart (MMC3-com")
	U8("patible)") U8("\x00") U8("Intellivision 10-in-1 Plug 'n Play 2nd")
	U8(" Edition") U8("\x00") U8("Techno Source") U8("\x00") U8("Super R")
	U8("ussian Roulette") U8("\x00") U8("9999999-in-1 multicart") U8("\x00")
	U8("Lucky Rabbit (0353) (FDS conversion of Roger Rabbit)") U8("\x00") U8("4")
	U8("-in-1 multicart") U8("\x00") U8("Batman: The Video Game (Fine St")
	U8("udio bootleg)") U8("\x00") U8("Fine Studio") U8("\x00") U8("8201")
	U8("06-C / 821007C (LH42)") U8("\x00") U8("TK-8007 MCU (VT03)") U8("\x00")
	U8("Taikee") U8("\x00") U8("A971210") U8("\x00") U8("Kasheng") U8("\x00")
	U8("SC871115C") U8("\x00") U8("BS-300 / BS-400 / BS-4040 multicarts") U8("\x00")
	U8("Lexibook Compact Cyber Arcade (VT369)") U8("\x00") U8("Lexibook") U8("\x00")
	U8("Lexibook Retro TV Game Console (VT369)") U8("\x00") U8("Cube Tec")
	U8("h VT369 handheld consoles") U8("\x00") U8("Cube Tech") U8("\x00") U8("V")
	U8("T369-based handheld console with 256-byte serial ROM") U8("\x00") U8("V")
	U8("T369-based handheld console with I") U8("\xc2\xb2") U8("C protec")
	U8("tion chip") U8("\x00") U8("(C)NROM multicart") U8("\x00") U8("Mi")
	U8("lowork FCFC1 flash cartridge (A mode)") U8("\x00") U8("Milowork") U8("\x00")
	U8("831031C/T-308 multicart (MMC3-based)") U8("\x00") U8("Realtec GN")
	U8("-91B") U8("\x00") U8("Realtec 8090 (MMC3-based)") U8("\x00") U8("N")
	U8("C-20MB PCB; 20-in-1 multicart (CA-006)") U8("\x00") U8("S-009") U8("\x00")
	U8("NC3000M multicart") U8("\x00") U8("NC7000M multicart (MMC3-compa")
	U8("tible)") U8("\x00") U8("Zh") U8("\xc5\x8d") U8("nggu") U8("\xc3\xb3")
	U8(" D") U8("\xc3\xa0") U8("h") U8("\xc4\x93") U8("ng") U8("\x00") U8("M")
	U8("\xc4\x9b") U8("i Sh") U8("\xc3\xa0") U8("on") U8("\xc7\x9a") U8(" ")
	U8("M") U8("\xc3\xa8") U8("ng G") U8("\xc5\x8d") U8("ngch") U8("\xc7")
	U8("\x8e") U8("ng III") U8("\x00") U8("Subor Karaoke") U8("\x00") U8("F")
	U8("amily Noraebang") U8("\x00") U8("Brilliant Com Cocoma Pack") U8("\x00")
	U8("EduBank") U8("\x00") U8("Kkachi-wa Nolae Chingu") U8("\x00") U8("S")
	U8("ubor multicart") U8("\x00") U8("UNL-EH8813A") U8("\x00") U8("2-i")
	U8("n-1 Datach multicart (VRC4e clone)") U8("\x00") U8("Korean Igo") U8("\x00")
	U8("F") U8("\xc5\xab") U8("un Sh") U8("\xc5\x8d") U8("rinken (FDS co")
	U8("nversion)") U8("\x00") U8("F") U8("\xc4\x93") U8("ngsh") U8("\xc3")
	U8("\xa9") U8("nb") U8("\xc7\x8e") U8("ng: F") U8("\xc3\xba") U8("m") U8("\xc3")
	U8("\xb3") U8(" S") U8("\xc4\x81") U8("n T") U8("\xc3\xa0") U8("iz") U8("\xc7")
	U8("\x90") U8(" (Jncota)") U8("\x00") U8("The Lord of King (Jaleco) ")
	U8("(bootleg)") U8("\x00") U8("UNL-KS7021A (VRC2b clone)") U8("\x00") U8("S")
	U8("angokushi: Ch") U8("\xc5\xab") U8("gen no Hasha (bootleg)") U8("\x00")
	U8("Fud") U8("\xc5\x8d") U8(" My") U8("\xc5\x8d\xc5\x8d") U8(" Den (")
	U8("bootleg) (VRC2b clone)") U8("\x00") U8("1995 New Series Super 2-")
	U8("in-1 multicart") U8("\x00") U8("Datach Dragon Ball Z (bootleg) (")
	U8("VRC4e clone)") U8("\x00") U8("Super Mario Bros. Pocker Mali (VRC")
	U8("4f clone)") U8("\x00") U8("LittleCom PC-95") U8("\x00") U8("Sach")
	U8("en 3014") U8("\x00") U8("2-in-1 Sudoku/Gomoku (NJ064) (MMC3 clon")
	U8("e)") U8("\x00") U8("Nazo no Murasamej") U8("\xc5\x8d") U8(" (FDS")
	U8(" conversion)") U8("\x00") U8("W") U8("\xc3\xa0") U8("ix") U8("\xc4")
	U8("\xab") U8("ng FS303 (MMC3 clone) (same as 195)") U8("\x00") U8("6")
	U8("0-1064-16L") U8("\x00") U8("Kid Icarus (FDS conversion)") U8("\x00")
	U8("Master Fighter VI' hack (variant of 359)") U8("\x00") U8("Little")
	U8("Com 160-in-1 multicart") U8("\x00") U8("World Hero hack (VRC4 cl")
	U8("one)") U8("\x00") U8("5-in-1 (CH-501) multicart (MMC1 clone)") U8("\x00")
	U8("W") U8("\xc3\xa0") U8("ix") U8("\xc4\xab") U8("ng FS306") U8("\x00")
	U8("Konami QTa adapter (VRC5)") U8("\x00") U8("CTC-15") U8("\x00") U8("C")
	U8("o Tung Co.") U8("\x00") U8("JY820845C") U8("\x00") U8("Jncota RP")
	U8("G re-release (variant of 178)") U8("\x00") U8("Taito X1-017 (cor")
	U8("rect PRG ROM bank ordering)") U8("\x00") U8("Sachen 3013") U8("\x00")
	U8("Kaiser KS-7010") U8("\x00") U8("Nintendo Campus Challenge 1991 (")
	U8("RetroUSB version)") U8("\x00") U8("JY-215 multicart") U8("\x00") U8("M")
	U8("oero TwinBee: Cinnamon-hakase o Sukue! (FDS conversion)") U8("\x00")
	U8("YC-03-09") U8("\x00") U8("Y") U8("\xc3\xa0") U8("nch") U8("\xc3\xa9")
	U8("ng") U8("\x00") U8("Bung Super Game Doctor 2M/4M RAM cartridge") U8("\x00")
	U8("Venus Turbo Game Doctor 4+/6+/6M RAM cartridge") U8("\x00");

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
	{10611, 2232, NESMirroring::Unknown},
	{10631, 2232, NESMirroring::Unknown},
	{10667, 3827, NESMirroring::Unknown},
	{10681, 0, NESMirroring::Unknown},
	{10698, 10724, NESMirroring::Unknown},
	{10732, 0, NESMirroring::Unknown},
	{10755, 3827, NESMirroring::Unknown},
	{10771, 0, NESMirroring::Unknown},

	/* Mapper 520 */
	{10783, 0, NESMirroring::Unknown},
	{10821, 0, NESMirroring::Unknown},
	{10832, 2710, NESMirroring::Unknown},
	{10866, 5292, NESMirroring::Unknown},
	{10912, 0, NESMirroring::Unknown},
	{10948, 3169, NESMirroring::Unknown},
	{10974, 0, NESMirroring::Unknown},
	{11013, 0, NESMirroring::Unknown},
	{11054, 0, NESMirroring::Unknown},
	{11093, 0, NESMirroring::Unknown},

	/* Mapper 530 */
	{11138, 0, NESMirroring::Unknown},
	{11182, 6663, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{11198, 2232, NESMirroring::Unknown},
	{11210, 0, NESMirroring::Unknown},
	{11252, 2710, NESMirroring::Unknown},
	{11289, 1442, NESMirroring::Unknown},
	{11289, 1442, NESMirroring::Unknown},
	{11332, 0, NESMirroring::Unknown},
	{11344, 0, NESMirroring::Unknown},

	/* Mapper 540 */
	{11372, 0, NESMirroring::Unknown},
	{11413, 0, NESMirroring::Unknown},
	{11442, 0, NESMirroring::Unknown},
	{11471, 0, NESMirroring::Unknown},
	{11510, 1442, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},
	{11526, 426, NESMirroring::Unknown},
	{11552, 11559, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 550 */
	{11571, 674, NESMirroring::Unknown},
	{11581, 5292, NESMirroring::Unknown},
	{11620, 621, NESMirroring::Unknown},
	{11665, 2232, NESMirroring::Unknown},
	{11677, 3169, NESMirroring::Unknown},
	{11692, 6, NESMirroring::Unknown},
	{11742, 674, NESMirroring::Unknown},
	{11759, 3169, NESMirroring::Unknown},
	{11816, 11825, NESMirroring::Unknown},
	{0, 0, NESMirroring::Unknown},

	/* Mapper 560 */
	{0, 0, NESMirroring::Unknown},
	{11836, 139, NESMirroring::Unknown},
	{11879, 0, NESMirroring::Unknown},
};
