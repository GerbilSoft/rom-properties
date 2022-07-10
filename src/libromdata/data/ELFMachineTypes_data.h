/** ELF Machine Types (generated from ELFMachineTypes_data.txt) **/

#include <stdint.h>

static const char8_t ELFMachineTypes_strtbl[] =
	U8("\x00") U8("No machine") U8("\x00") U8("AT&T WE 32100 (M32)") U8("\x00")
	U8("Sun/Oracle SPARC") U8("\x00") U8("Intel i386") U8("\x00") U8("Mo")
	U8("torola M68K") U8("\x00") U8("Motorola M88K") U8("\x00") U8("Inte")
	U8("l i486") U8("\x00") U8("Intel i860") U8("\x00") U8("MIPS") U8("\x00")
	U8("IBM System/370") U8("\x00") U8("MIPS R3000 LE (deprecated)") U8("\x00")
	U8("SPARC v9 (deprecated)") U8("\x00") U8("HP PA-RISC") U8("\x00") U8("n")
	U8("CUBE") U8("\x00") U8("Fujitsu VPP500") U8("\x00") U8("SPARC32PLU")
	U8("S") U8("\x00") U8("Intel i960") U8("\x00") U8("PowerPC") U8("\x00")
	U8("64-bit PowerPC") U8("\x00") U8("IBM System/390") U8("\x00") U8("C")
	U8("ell SPU") U8("\x00") U8("Cisco SVIP") U8("\x00") U8("Cisco 7200") U8("\x00")
	U8("NEC V800") U8("\x00") U8("Fujitsu FR20") U8("\x00") U8("TRW RH-3")
	U8("2") U8("\x00") U8("Motorola M*Core") U8("\x00") U8("ARM") U8("\x00")
	U8("DEC Alpha") U8("\x00") U8("Renesas SuperH") U8("\x00") U8("SPARC")
	U8(" v9") U8("\x00") U8("Siemens Tricore embedded processor") U8("\x00")
	U8("Argonaut RISC Core") U8("\x00") U8("Renesas H8/300") U8("\x00") U8("R")
	U8("enesas H8/300H") U8("\x00") U8("Renesas H8S") U8("\x00") U8("Ren")
	U8("esas H8/500") U8("\x00") U8("Intel Itanium") U8("\x00") U8("Stan")
	U8("ford MIPS-X") U8("\x00") U8("Motorola Coldfire") U8("\x00") U8("M")
	U8("otorola MC68HC12") U8("\x00") U8("Fujitsu Multimedia Accelerator")
	U8("\x00") U8("Siemens PCP") U8("\x00") U8("Sony nCPU") U8("\x00") U8("D")
	U8("enso NDR1") U8("\x00") U8("Motorola Star*Core") U8("\x00") U8("T")
	U8("oyota ME16") U8("\x00") U8("STMicroelectronics ST100") U8("\x00") U8("A")
	U8("dvanced Logic Corp. TinyJ") U8("\x00") U8("AMD64") U8("\x00") U8("S")
	U8("ony DSP") U8("\x00") U8("DEC PDP-10") U8("\x00") U8("DEC PDP-11") U8("\x00")
	U8("Siemens FX66") U8("\x00") U8("STMicroelectronics ST9+ 8/16-bit") U8("\x00")
	U8("STMicroelectronics ST7 8-bit") U8("\x00") U8("Motorola MC68HC16") U8("\x00")
	U8("Motorola MC68HC11") U8("\x00") U8("Motorola MC68HC08") U8("\x00") U8("M")
	U8("otorola MC68HC05") U8("\x00") U8("SGI SVx or Cray NV1") U8("\x00")
	U8("STMicroelectronics ST19 8-bit") U8("\x00") U8("Digital VAX") U8("\x00")
	U8("Axis cris") U8("\x00") U8("Infineon Technologies 32-bit embedded")
	U8(" CPU") U8("\x00") U8("Element 14 64-bit DSP") U8("\x00") U8("LSI")
	U8(" Logic 16-bit DSP") U8("\x00") U8("Donald Knuth's 64-bit MMIX CP")
	U8("U") U8("\x00") U8("Harvard machine-independent") U8("\x00") U8("S")
	U8("iTera Prism") U8("\x00") U8("Atmel AVR 8-bit") U8("\x00") U8("Fu")
	U8("jitsu FR30") U8("\x00") U8("Mitsubishi D10V") U8("\x00") U8("Mit")
	U8("subishi D30V") U8("\x00") U8("Renesas V850") U8("\x00") U8("Rene")
	U8("sas M32R") U8("\x00") U8("Matsushita MN10300") U8("\x00") U8("Ma")
	U8("tsushita MN10200") U8("\x00") U8("picoJava") U8("\x00") U8("Open")
	U8("RISC 1000") U8("\x00") U8("ARCompact") U8("\x00") U8("Tensilica ")
	U8("Xtensa") U8("\x00") U8("Alphamosaic VideoCore") U8("\x00") U8("T")
	U8("hompson Multimedia GPP") U8("\x00") U8("National Semiconductor 3")
	U8("2000") U8("\x00") U8("Tenor Network TPC") U8("\x00") U8("Trebia ")
	U8("SNP 1000") U8("\x00") U8("STMicroelectronics ST200") U8("\x00") U8("U")
	U8("bicom IP2022") U8("\x00") U8("MAX Processor") U8("\x00") U8("Nat")
	U8("ional Semiconductor CompactRISC") U8("\x00") U8("Fujitsu F2MC16") U8("\x00")
	U8("TI msp430") U8("\x00") U8("ADI Blackfin") U8("\x00") U8("S1C33 F")
	U8("amily of Seiko Epson") U8("\x00") U8("Sharp embedded") U8("\x00") U8("A")
	U8("rca RISC") U8("\x00") U8("Unicore") U8("\x00") U8("eXcess") U8("\x00")
	U8("Icera Deep Execution Processor") U8("\x00") U8("Altera Nios II") U8("\x00")
	U8("National Semiconductor CRX") U8("\x00") U8("Motorola XGATE") U8("\x00")
	U8("Infineon C16x/XC16x") U8("\x00") U8("Renesas M16C series") U8("\x00")
	U8("Microchip dsPIC30F") U8("\x00") U8("Freescale RISC core") U8("\x00")
	U8("Renesas M32C series") U8("\x00") U8("Altium TSK3000 core") U8("\x00")
	U8("Freescale RS08") U8("\x00") U8("ADI SHARC family") U8("\x00") U8("C")
	U8("yan Technology eCOG2") U8("\x00") U8("Sunplus S+core7 RISC") U8("\x00")
	U8("New Japan Radio (NJR) 24-bit DSP") U8("\x00") U8("Broadcom Video")
	U8("Core III") U8("\x00") U8("Lattice Mico32") U8("\x00") U8("Seiko ")
	U8("Epson C17 family") U8("\x00") U8("TI TMS320C6000 DSP family") U8("\x00")
	U8("TI TMS320C2000 DSP family") U8("\x00") U8("TI TMS320C55x DSP fam")
	U8("ily") U8("\x00") U8("TI Application-Specific RISC") U8("\x00") U8("T")
	U8("I Programmable Realtime Unit") U8("\x00") U8("STMicroelectronics")
	U8(" 64-bit VLIW DSP") U8("\x00") U8("Cypress M8C") U8("\x00") U8("R")
	U8("enesas R32C series") U8("\x00") U8("NXP TriMedia family") U8("\x00")
	U8("Qualcomm DSP6") U8("\x00") U8("Intel 8051") U8("\x00") U8("STMic")
	U8("roelectronics STxP7x family") U8("\x00") U8("Andes Technology ND")
	U8("S32") U8("\x00") U8("Cyan eCOG1X family") U8("\x00") U8("Dallas ")
	U8("MAXQ30") U8("\x00") U8("New Japan Radio (NJR) 16-bit DSP") U8("\x00")
	U8("M2000 Reconfigurable RISC") U8("\x00") U8("Cray NV2 vector archi")
	U8("tecture") U8("\x00") U8("Renesas RX family") U8("\x00") U8("Imag")
	U8("ination Technologies Meta") U8("\x00") U8("MCST Elbrus") U8("\x00")
	U8("Cyan Technology eCOG16 family") U8("\x00") U8("National Semicond")
	U8("uctor CompactRISC (16-bit)") U8("\x00") U8("Freescale Extended T")
	U8("ime Processing Unit") U8("\x00") U8("Infineon SLE9X") U8("\x00") U8("I")
	U8("ntel L10M") U8("\x00") U8("Intel K10M") U8("\x00") U8("Intel (18")
	U8("2)") U8("\x00") U8("ARM AArch64") U8("\x00") U8("ARM (184)") U8("\x00")
	U8("Atmel AVR32") U8("\x00") U8("STMicroelectronics STM8 8-bit") U8("\x00")
	U8("Tilera TILE64") U8("\x00") U8("Tilera TILEPro") U8("\x00") U8("X")
	U8("ilinx MicroBlaze 32-bit RISC") U8("\x00") U8("NVIDIA CUDA") U8("\x00")
	U8("Tilera TILE-Gx") U8("\x00") U8("CloudShield") U8("\x00") U8("KIP")
	U8("O-KAIST Core-A 1st gen.") U8("\x00") U8("KIPO-KAIST Core-A 2nd g")
	U8("en.") U8("\x00") U8("Synopsys ARCompact V2") U8("\x00") U8("Open")
	U8("8 RISC") U8("\x00") U8("Renesas RL78 family") U8("\x00") U8("Bro")
	U8("adcom VideoCore V") U8("\x00") U8("Renesas 78K0R") U8("\x00") U8("F")
	U8("reescale 56800EX") U8("\x00") U8("Beyond BA1") U8("\x00") U8("Be")
	U8("yond BA2") U8("\x00") U8("XMOS xCORE") U8("\x00") U8("Micrchip 8")
	U8("-bit PIC(r)") U8("\x00") U8("Intel Graphics Technology") U8("\x00")
	U8("Intel (206)") U8("\x00") U8("Intel (207)") U8("\x00") U8("Intel ")
	U8("(208)") U8("\x00") U8("Intel (209)") U8("\x00") U8("KM211 KM32") U8("\x00")
	U8("KM211 KMX32") U8("\x00") U8("KM211 KMX16") U8("\x00") U8("KM211 ")
	U8("KMX8") U8("\x00") U8("KM211 KVARC") U8("\x00") U8("Paneve CDP") U8("\x00")
	U8("Cognitive Smart Memory") U8("\x00") U8("Bluechip Systems CoolEng")
	U8("ine") U8("\x00") U8("Nanoradio Optimized RISC") U8("\x00") U8("C")
	U8("SR Kalimba") U8("\x00") U8("Zilog Z80") U8("\x00") U8("Controls ")
	U8("and Data Services VISIUMcore") U8("\x00") U8("FTDI Chip FT32") U8("\x00")
	U8("Moxie processor") U8("\x00") U8("AMD GPU") U8("\x00") U8("RISC-V")
	U8("\x00") U8("Lanai") U8("\x00") U8("Linux eBPF") U8("\x00") U8("Ne")
	U8("tronome Flow Processor") U8("\x00") U8("NEC VE") U8("\x00") U8("C")
	U8("-SKY") U8("\x00") U8("LoongArch") U8("\x00");

static const uint16_t ELFMachineTypes_offtbl[] = {
	/* ELF Machine Type 0 */
	1,12,32,49,60,74,88,99,
	110,115,130,157,0,0,0,179,
	190,196,211,223,234,242,257,272,
	281,292,0,0,0,0,0,0,

	/* ELF Machine Type 32 */
	0,0,0,0,303,312,325,335,
	351,355,365,380,389,424,443,458,
	474,486,501,515,531,549,567,598,
	610,620,631,650,662,687,714,720,

	/* ELF Machine Type 64 */
	729,740,751,764,797,826,844,862,
	880,898,918,948,960,970,1012,1034,
	1055,1086,1114,1127,1143,1156,1172,1188,
	1201,1214,1233,1252,1261,1275,1285,1302,

	/* ELF Machine Type 96 */
	1324,1348,1377,1395,1411,1436,1450,1464,
	1499,1514,1524,1537,1565,1580,1590,1598,
	1605,1636,1651,1678,1693,1713,1733,1752,
	1772,0,0,0,0,0,0,0,

	/* ELF Machine Type 128 */
	0,0,0,1792,1812,1827,1844,1866,
	1887,1920,1943,1958,1981,2007,2033,2058,
	2087,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,

	/* ELF Machine Type 160 */
	2117,2152,2164,2184,2204,2218,2229,2262,
	2285,2304,2318,2351,2377,2406,2424,2454,
	2466,2496,2540,2580,2595,2606,2617,2629,
	2641,2651,2663,2693,2707,2722,2752,2764,

	/* ELF Machine Type 192 */
	2779,2791,2818,2845,2867,2878,2898,2919,
	2933,2951,2962,2973,2984,3006,3032,3044,
	3056,3068,3080,3091,3103,3115,3126,3138,
	3149,3172,3200,3225,3237,3247,3285,3300,

	/* ELF Machine Type 224 */
	3316,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,3324,3331,0,0,3337,
	0,0,3348,3373,3380,0,0,0,

	/* ELF Machine Type 256 */
	0,0,3386,
};
