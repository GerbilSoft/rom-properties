/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * vgm_structs.h: VGM audio data structures.                               *
 *                                                                         *
 * Copyright (c) 2018-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - http://vgmrips.net/wiki/VGM_Specification
// - http://vgmrips.net/wiki/GD3_Specification

#pragma once

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Video Game Music Format.
 *
 * All fields are in little-endian, except for the
 * magic number, which is considered "big-endian".
 *
 * All pointer offsets are relative to that field's address.
 */
#define VGM_MAGIC 'Vgm '
#define VGM_SAMPLE_RATE 44100U	// All VGM sample values use this rate.
#define VGM_CLK_FLAG_ALTMODE	(1U << 31)	/* alternate mode for some sound chips */
#define VGM_CLK_FLAG_DUALCHIP	(1U << 30)	/* dual-chip mode for some sound chips */
#define PSG_T6W28 (VGM_CLK_FLAG_ALTMODE | VGM_CLK_FLAG_DUALCHIP)
typedef struct _VGM_Header {
	uint32_t magic;		// [0x000] "Vgm "
	uint32_t eof_offset;	// [0x004] Offset: End of file.
	uint32_t version;	// [0x008] Version number, in BCD.

	// Clock rates, in Hz.
	// If the corresponding chip is not present, set to 0.
	uint32_t sn76489_clk;	// [0x00C] SN76489 clock rate. (Typ: 3,579,545)
	uint32_t ym2413_clk;	// [0x010] YM2413 clock rate. (Typ: 3,579,545)
				// Bit 30: Dual PSGs.
				// Bit 31: T6W28 (NGPC) (requires Dual PSGs)

	uint32_t gd3_offset;	// [0x014] GD3 offset.
	uint32_t sample_count;	// [0x018] Total number of samples. (wait values)
	uint32_t loop_offset;	// [0x01C] Loop point offset. (0 if no loop)
	uint32_t loop_samples;	// [0x020] Number of samples in one loop. (0 if no loop)

	/** VGM 1.01 **/
	uint32_t frame_rate;	// [0x024] "Framerate" of the corresponding console if
				//         rate scaling is needed. (Set to 0 for none.)
				//         Set to 60 for NTSC, 50 for PAL.

	/** VGM 1.10 **/
	uint16_t sn76489_lfsr;	// [0x028] SN76489 LFSR pattern. (See VGM_LFSR_e.)
	uint8_t sn76489_width;	// [0x02A] SN76489 shift regsiter width. (See VGM_ShiftW_e.)

	/** VGM 1.51 **/
	uint8_t sn76489_flags;	// [0x02B] SN76489 flags. (See VGM_Flags_SN76489_e.)

	/** VGM 1.10 **/
	uint32_t ym2612_clk;	// [0x02C] YM2612 clock rate. (Typ: 7,670,454)
				//         If v1.01 or earlier, use ym2413_clk.
	uint32_t ym2151_clk;	// [0x030] YM2151 clock rate. (Typ: 3,579,545)
				//         If v1.01 or earlier, use ym2413_clk.

	/** VGM 1.50 **/
	uint32_t data_offset;	// [0x034] Offset to VGM data stream.
				//         If earlier than v1.50, VGM data starts at
				//         relative offset 0x0C (0x40).

	/** VGM 1.51 **/
	uint32_t sega_pcm_clk;		// [0x038] Sega PCM clock rate. (Typ: 4,000,000)
	uint32_t sega_pcm_if_reg;	// [0x03C] Sega PCM interface register.
	uint32_t rf5c68_clk;		// [0x040] RF5C68 clock rate. (Typ: 12,500,000)
	uint32_t ym2203_clk;		// [0x044] YM2203 clock rate. (Typ: 3,000,000)
	uint32_t ym2608_clk;		// [0x048] YM2608 clock rate. (Typ: 8,000,000)
	uint32_t ym2610_clk;		// [0x04C] YM2610/YM2610B clock rate. (Typ: 8,000,000)
					//         Bit 31: 0=YM2610, 1=YM2610B
	uint32_t ym3812_clk;		// [0x050] YM3812 clock rate. (Typ: 3,579,545)
	uint32_t ym3526_clk;		// [0x054] YM3526 clock rate. (Typ: 3,579,545)
	uint32_t y8950_clk;		// [0x058] Y8950 clock rate. (Typ: 3,579,545)
	uint32_t ymf262_clk;		// [0x05C] YMF262 clock rate. (Typ: 14,318,180)
	uint32_t ymf278b_clk;		// [0x060] YMF278B clock rate. (Typ: 33,868,800)
	uint32_t ymf271_clk;		// [0x064] YMF271 clock rate. (Typ: 16,934,400)
	uint32_t ymz280b_clk;		// [0x068] YMF280B clock rate. (Typ: 16,934,400)
	uint32_t rf5c164_clk;		// [0x06C] RF5C164 clock rate. (Typ: 12,500,000)
	uint32_t pwm_clk;		// [0x070] PWM clock rate. (Typ: 23,011,361)
	uint32_t ay8910_clk;		// [0x074] AY8910 clock rate. (Typ: 1,789,750)
	uint8_t ay8910_type;		// [0x078] AY8910 type. (See VGM_AY_8910_Type_e.)
	uint8_t ay8910_flags;		// [0x079] AY8910 flags. (See VGM_AY8910_Flags_e.)
	uint8_t ym2203_ay8910_flags;	// [0x07A] YM2203's AY8910 flags.
	uint8_t ym2608_ay8910_flags;	// [0x07B] YM2608's AY8910 flags.

	/** VGM 1.60 **/
	uint8_t vol_modifier;		// [0x07C] Volume = pow(2, (vol_modifier / 0x20))
					//         vol_modifier is from -63 (0xC1) to 192 (0xC0);
					//         -63 is interpreted as -64. Range is (0.25, 64).
					//         Default is 0, which is a factor of 1. (100%)
					//         NOTE: Should be supported in v1.50 for MD VGMs.
	uint8_t reserved_160;		// [0x07D]
	int8_t loop_base;		// [0x07E] Modifies the number of loops that are played
					//         before playback ends.

	/** VGM 1.51 **/
	uint8_t loop_modifier;		// [0x07F] Modifies the number of loops:
					//         num_loops = program_num_loops * loop_modifier / 0x10
					//         Default is 0, which is equal to 0x10.

	/** VGM 1.61 **/
	uint32_t dmg_clk;		// [0x080] Game Boy LR35902 clock rate. (Typ: 4,194,304)
	uint32_t nes_apu_clk;		// [0x084] NES APU (2A03) clock rate. (Typ: 1,789,772)
					//         Bit 31: Set to 1 if FDS is connected.
	uint32_t multipcm_clk;		// [0x088] MultiPCM clock rate. (Typ: 8,053,975)
	uint32_t upd7759_clk;		// [0x08C] uPD7759 clock rate. (Typ: 640,000)
	uint32_t okim6258_clk;		// [0x090] OKIM6258 clock rate. (Typ: 4,000,000)
	uint8_t okim6258_flags;		// [0x094] OKIM6258 flags. (See VGM_OKIM6258_Flags_e.)
	uint8_t k054539_flags;		// [0x095] K054539 flags. (See VGM_K054539_Flags_e.)
	uint8_t c140_chip_type;		// [0x096] C140 chip type. (See VGM_C140_Type_e.)
	uint8_t reserved_161;		// [0x097]
	uint32_t okim6295_clk;		// [0x098] OKIM6295 clock rate. (Typ: 8,000,000)
	uint32_t k051649_clk;		// [0x09C] K051649 clock rate. (Typ: 1,500,000)
	uint32_t k054539_clk;		// [0x0A0] K054539 clock rate. (Typ: 18,432,000)
	uint32_t huc6280_clk;		// [0x0A4] HuC6280 clock rate. (Typ: 3,579,545)
	uint32_t c140_clk;		// [0x0A8] C140 clock rate. (Typ: 21,390)
	uint32_t k053260_clk;		// [0x0AC] K053260 clock rate. (Typ: 3,579,545)
	uint32_t pokey_clk;		// [0x0B0] Atari POKEY clock rate. (Typ: 1,789,772)
	uint32_t qsound_clk;		// [0x0B4] QSound clock rate. (Typ: 4,000,000)

	/** VGM 1.71 **/
	uint32_t scsp_clk;		// [0x0B8] SCSP clock rate. (Typ: 22,579,200)

	/** VGM 1.70 **/
	uint32_t exheader_offset;	// [0x0BC] Extra header offset. (0 if not present.)

	/** VGM 1.71 **/
	uint32_t ws_clk;		// [0x0C0] WonderSwan clock rate. (Typ: 3,072,000)
	uint32_t vsu_clk;		// [0x0C4] VSU clock rate. (Typ: 5,000,000)
	uint32_t saa1099_clk;		// [0x0C8] SAA1099 clock rate. (Typ: 8,000,000; 7,159,000; 7,159,090)
	uint32_t es5503_clk;		// [0x0CC] ES5503 clock rate. (Typ: 7,159,090)
	uint32_t es5505_clk;		// [0x0D0] ES5505/ES5506 clock rate.
					//         Bit 31: 0=ES5505, 1=ES5506
	uint8_t es5503_num_ch;		// [0x0D4] ES5503: Number of internal channels. (1-8)
					//         Typical value: 2
	uint8_t es5505_num_ch;		// [0x0D5] ES5505/ES5506: Number of internal channels.
					//         ES5505: 1-4; ES5506: 1-8 - typical value: 1
	uint8_t c352_clk_div;		// [0x0D6] C352 clock divider. (0-1020; multiply this by 4)
					//         Typical value: 288
	uint8_t reserved_171_a;		// [0x0D7]
	uint32_t x1_010_clk;		// [0x0D8] X1-010 clock rate. (Typ: 16,000,000)
	uint32_t c352_clk;		// [0x0DC] C352 clock rate. (Typ: 24,192,000)
	uint32_t ga20_clk;		// [0x0E0] GA20 clock rate. (Typ: 3,579,545)
	uint8_t reserved_171_b[4];	// [0x0E4]
} VGM_Header;
ASSERT_STRUCT(VGM_Header, 232);

/**
 * GD3 header
 */
#define GD3_MAGIC 'Gd3 '
typedef struct _GD3_Header {
	uint32_t magic;		// [0x000] "Gd3 "
	uint32_t version;	// [0x004] Version number, in BCD. (v1.00)
	uint32_t length;	// [0x008] Length of the GD3 data.
} GD3_Header;
ASSERT_STRUCT(GD3_Header, 3*sizeof(uint32_t));

/**
 * GD3 tag indexes
 */
typedef enum {
	GD3_TAG_TRACK_NAME_EN		= 0,
	GD3_TAG_TRACK_NAME_JP		= 1,
	GD3_TAG_GAME_NAME_EN		= 2,
	GD3_TAG_GAME_NAME_JP		= 3,
	GD3_TAG_SYSTEM_NAME_EN		= 4,
	GD3_TAG_SYSTEM_NAME_JP		= 5,
	GD3_TAG_TRACK_AUTHOR_EN		= 6,
	GD3_TAG_TRACK_AUTHOR_JP		= 7,
	GD3_TAG_DATE_GAME_RELEASE	= 8,
	GD3_TAG_VGM_RIPPER		= 9,
	GD3_TAG_NOTES			= 10,

	GD3_TAG_MAX
} GD3_TAG_ID;

/**
 * VGM 1.10: SN76489 LFSR patterns.
 */
typedef enum {
	VGM_LFSR_SMS2		= 0x0009,	// SMS2, Game Gear, Mega Drive (default)
	VGM_LFSR_SN76489AN	= 0x0003,	// SN76489AN (SC-3000H, BBC Micro)
	VGM_LFSR_SN76494	= 0x0006,	// SN76494
	VGM_LFSR_SN76496	= 0x0006,	// SN76496
} VGM_LFSR_e;

/**
 * VGM 1.10: SN76489 shift register width.
 */
typedef enum {
	VGM_SHIFTW_SMS2		= 16,		// SMS2, Game Gear, Mega Drive (default)
	VGM_SHIFTW_SN76489AN	= 15,		// SN76489AN (SC-3000H, BBC Micro)
} VGM_ShiftW_e;

/**
 * VGM 1.51: SN76489 flags.
 * NOTE: This is a bitfield.
 */
typedef enum {
	VGM_FLAG_SN76489_FREQ0_0x400	= (1U << 0),	// Frequency 0 is 0x400
	VGM_FLAG_SN76489_OUTPUT_NEGATE	= (1U << 1),	// Negate output
	VGM_FLAG_SN76489_STEREO		= (1U << 2),	// Stereo enable (0 == enabled)
	VGM_FLAG_SN76489_CLOCK_DIV8	= (1U << 3),	// /8 Clock Divider (0 == enabled)
} VGM_Flags_SN76489_e;

/**
 * VGM 1.51: AY8910 chip type.
 */
typedef enum {
	VGM_AY8910_TYPE_AY8910	= 0x00,		// AY8910
	VGM_AY8910_TYPE_AY8912	= 0x01,		// AY8912
	VGM_AY8910_TYPE_AY8913	= 0x02,		// AY8913
	VGM_AY8910_TYPE_AY8930	= 0x03,		// AY8930
	VGM_AY8910_TYPE_YM2149	= 0x10,		// YM2149
	VGM_AY8910_TYPE_YM3439	= 0x11,		// YM3439
	VGM_AY8910_TYPE_YMZ284	= 0x12,		// YMZ284
	VGM_AY8910_TYPE_YMZ294	= 0x13,		// YMZ294
} VGM_AY8910_Type_e;

/**
 * VGM 1.51: AY8910 chip flags.
 * Default is 0x01.
 * NOTE: This is a bitfield.
 */
typedef enum {
	VGM_AY8910_FLAG_LEGACY_OUTPUT	= (1U << 0),
	VGM_AY8910_FLAG_SINGLE_OUTPUT	= (1U << 1),
	VGM_AY8910_FLAG_DISCRETE_OUTPUT	= (1U << 2),
	VGM_AY8910_FLAG_RAW_OUTPUT	= (1U << 3),
} VGM_AY8910_Flags_e;

/**
 * VGM 1.61: OKIM6258 flags.
 * Default is 0x00.
 * NOTE: This is a bitfield.
 */
typedef enum {
	VGM_OKIM6258_FLAG_CLKDIV_MASK	= 3,	// Clock divider mask.
	VGM_OKIM6258_FLAG_CLKDIV_1024	= 0,
	VGM_OKIM6258_FLAG_CLKDIV_768	= 1,
	VGM_OKIM6258_FLAG_CLKDIV_512	= 2,	// also 3

	VGM_OKIM6258_FLAG_ADPCM_BITS	= (1U << 2),	// 0 == 4-bit ADPCM; 1 == 3-bit ADPCM
	VGM_OKIM6258_FLAG_10_12_BIT	= (1U << 3),	// 0 == 10-bit output; 1 == 12-bit output
} VGM_OKIM6258_Flags_e;

/**
 * VGM 1.61: K054539 flags.
 * Default is 0x01.
 * NOTE: This is a bitfield.
 */
typedef enum {
	VGM_K054539_FLAG_REVERSE_STEREO		= (1U << 0),	// Reverse stereo. (1=ON; 0=OFF)
	VGM_K054539_FLAG_DISABLE_REVERB		= (1U << 1),	// Disable reverb.
	VGM_K054539_FLAG_UPDATE_AT_KEY_ON	= (1U << 2),	// Update at KeyOn.
} VGM_K054539_Flags_e;

/**
 * VGM 1.61: C140 chip type.
 */
typedef enum {
	VGM_C140_TYPE_NAMCO_SYSTEM_2	= 0x00,		// C140, Namco System 2
	VGM_C140_TYPE_NAMCO_SYSTEM_21	= 0x01,		// C140, Namco System 21
	VGM_C140_TYPE_NAMCO_C219	= 0x02,		// C219, Namco NA-1/NA-2
} VGM_C140_Type_e;

/**
 * VGM 1.70: Extra Header struct.
 * Indicates additional chip clocks and volumes for
 * systems with multiples of the same chips.
 */
typedef struct _VGM_170_ExtraHeader {
	uint32_t chpclock_offset;	// [0x000] Offset to chip clocks.
	uint32_t chpvol_offset;		// [0x004] Offset to chip volumes.
} VGM_170_ExtraHeader;
ASSERT_STRUCT(VGM_170_ExtraHeader, 2*sizeof(uint32_t));

/**
 * VGM 1.70: Chip Clock entry.
 */
#pragma pack(1)
typedef struct RP_PACKED _VGM_170_ChipClock {
	uint8_t chip_id;		// [0x000] Chip ID.
	uint32_t clock_rate;		// [0x001] Clock rate.
} VGM_170_ChipClock;
ASSERT_STRUCT(VGM_170_ChipClock, 5);
#pragma pack()

/**
 * VGM 1.70: Chip Volume entry.
 */
typedef struct _VGM_170_ChipVolume {
	uint8_t chip_id;	// [0x000] Chip ID.
				// If bit 7 is set, it's the volume for a paired chip,
				// e.g. the AY- part of the YM2203.
	uint8_t flags;		// [0x001] Flags.
				// If bit 0 is set, it's the volume for the second chip.
	uint16_t volume;	// [0x002] Volume.
				// If bit 15 is 0, this is an absolute volume setting.
				// Otherwise, it's relative, and the chip value gets
				// multiplied by ((value & 0x7FFF) / 0x0100).
} VGM_170_ChipVolume;
ASSERT_STRUCT(VGM_170_ChipVolume, 4);

#ifdef __cplusplus
}
#endif
