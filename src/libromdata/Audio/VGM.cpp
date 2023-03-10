/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * VGM.hpp: VGM audio reader.                                              *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "VGM.hpp"
#include "vgm_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;
using LibRpFile::IRpFile;

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class VGMPrivate final : public RomDataPrivate
{
	public:
		VGMPrivate(IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(VGMPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// VGM header.
		// NOTE: **NOT** byteswapped in memory.
		VGM_Header vgmHeader;

		// Translated strings for addCommonSoundChip().
		const char *s_clockrate;
		const char *s_dualchip;
		const char *s_yes;
		const char *s_no;

	public:
		// GD3 tags.
		// All strings must be in UTF-8 format.
		typedef array<string, GD3_TAG_MAX> gd3_tags_t;

		/**
		 * Load GD3 tags.
		 * @param addr Starting address of the GD3 tag block.
		 * @return Allocated GD3 tags, or nullptr on error.
		 */
		gd3_tags_t *loadGD3(unsigned int addr);

		/**
		 * Add a common sound chip field.
		 * @param clk Clock value. (top two bits are DUAL and )
		 * @param display Display name.
		 * @param dual If true, dual-chip mode is supported.
		 */
		void addCommonSoundChip(unsigned int clk, const char *display, bool dual = false);
};

ROMDATA_IMPL(VGM)

/** VGMPrivate **/

/* RomDataInfo */
const char *const VGMPrivate::exts[] = {
	".vgm",
	".vgz",	// gzipped
	//".vgm.gz",	// NOTE: Windows doesn't support this.

	nullptr
};
const char *const VGMPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	"audio/x-vgm",

	nullptr
};
const RomDataInfo VGMPrivate::romDataInfo = {
	"VGM", exts, mimeTypes
};

VGMPrivate::VGMPrivate(IRpFile *file)
	: super(file, &romDataInfo)
	, s_clockrate(nullptr)
	, s_dualchip(nullptr)
	, s_yes(nullptr)
	, s_no(nullptr)
{
	// Clear the VGM header struct.
	memset(&vgmHeader, 0, sizeof(vgmHeader));
}

/**
 * Load GD3 tags.
 * @param addr Starting address of the GD3 tag block.
 * @return Allocated GD3 tags, or nullptr on error.
 */
VGMPrivate::gd3_tags_t *VGMPrivate::loadGD3(unsigned int addr)
{
	assert(file != nullptr);
	assert(file->isOpen());
	if (!file || !file->isOpen()) {
		return nullptr;
	}

	GD3_Header gd3Header;
	size_t size = file->seekAndRead(addr, &gd3Header, sizeof(gd3Header));
	if (size != sizeof(gd3Header)) {
		// Seek and/or read error.
		return nullptr;
	}

	// Validate the header.
	if (gd3Header.magic != cpu_to_be32(GD3_MAGIC) ||
	    le32_to_cpu(gd3Header.version) < 0x0100)
	{
		// Incorrect header.
		// TODO: Require exactly v1.00?
		return nullptr;
	}

	// Length limitations:
	// - Must be an even number. (UTF-16)
	// - Minimum of 11*2 bytes; Maximum of 16 KB.
	const unsigned int length = le32_to_cpu(gd3Header.length);
	if (length % 2 != 0 || length < 11*2 || length > 16*1024) {
		// Incorrect length value.
		return nullptr;
	}

	// Read the GD3 data.
	const unsigned int length16 = length / 2;
	unique_ptr<char16_t[]> gd3(new char16_t[length16]);
	size = file->read(gd3.get(), length);
	if (size != length) {
		// Read error.
		return nullptr;
	}

	// Make sure the end of the GD3 data is NULL-terminated.
	if (gd3[length16-1] != 0) {
		// Not NULL-terminated.
		return nullptr;
	}

	gd3_tags_t *gd3_tags = new gd3_tags_t;

	// Convert from NULL-terminated strings to gd3_tags_t.
	size_t tag_idx = 0;
	const char16_t *start = gd3.get();
	const char16_t *const endptr = start + length16;
	for (const char16_t *p = start; p < endptr && tag_idx < gd3_tags->size(); p++) {
		// Check for a NULL.
		if (*p == 0) {
			// Found a NULL!
			(*gd3_tags)[tag_idx] = utf16le_to_utf8(start, (int)(p-start));
			// Next string.
			start = p + 1;
			tag_idx++;
		}
	}

	// TODO: Return an error if there's more than GD3_TAG_MAX strings?
	return gd3_tags;
}

/**
 * Add a common sound chip field.
 * @param dual If true, dual-chip mode is supported.
 * @param clk_full Clock value. (top two bits are ALTMODE and possibly DUALCHIP)
 * @param display Display name.
 */
void VGMPrivate::addCommonSoundChip(unsigned int clk_full, const char *display, bool dual)
{
	unsigned int clk = clk_full;
	if (dual) {
		clk &= ~(VGM_CLK_FLAG_ALTMODE | VGM_CLK_FLAG_DUALCHIP);
	} else {
		clk &= ~VGM_CLK_FLAG_ALTMODE;
	}

	if (clk != 0) {
		fields.addField_string(
			rp_sprintf(s_clockrate, display).c_str(),
			LibRpText::formatFrequency(clk));
		if (dual) {
			fields.addField_string(
				rp_sprintf(s_dualchip, display).c_str(),
					(clk_full & VGM_CLK_FLAG_DUALCHIP) ? s_yes : s_no);
		}
	}
}

/** VGM **/

/**
 * Read an VGM audio file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
VGM::VGM(IRpFile *file)
	: super(new VGMPrivate(file))
{
	RP_D(VGM);
	d->mimeType = "audio/x-vgm";	// unofficial
	d->fileType = FileType::AudioFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the VGM header.
	d->file->rewind();
	size_t size = d->file->read(&d->vgmHeader, sizeof(d->vgmHeader));
	if (size != sizeof(d->vgmHeader)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(d->vgmHeader), reinterpret_cast<const uint8_t*>(&d->vgmHeader)},
		nullptr,	// ext (not needed for VGM)
		0		// szFile (not needed for VGM)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		UNREF_AND_NULL_NOCHK(d->file);
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int VGM::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(VGM_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const VGM_Header *const vgmHeader =
		reinterpret_cast<const VGM_Header*>(info->header.pData);

	// Check the VGM magic number.
	if (vgmHeader->magic == cpu_to_be32(VGM_MAGIC)) {
		// Found the VGM magic number.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *VGM::systemName(unsigned int type) const
{
	RP_D(const VGM);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// VGM has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"VGM::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Video Game Music", "VGM", "VGM", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int VGM::loadFieldData(void)
{
	RP_D(VGM);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// VGM header
	const VGM_Header *const vgmHeader = &d->vgmHeader;
	// NOTE: It's very unlikely that a single VGM will have
	// all supported sound chips, so we'll reserve enough
	// fields for the base data and up to 8 sound chips,
	// assuming 2 fields per chip.
	d->fields.reserve(11+(8*2));
	d->fields.reserveTabs(2);

	// Main tab
	d->fields.setTabName(0, "VGM");

	// Version number (BCD)
	unsigned int vgm_version = le32_to_cpu(vgmHeader->version);
	d->fields.addField_string(C_("VGM", "VGM Version"),
		rp_sprintf("%x.%02x", vgm_version >> 8, vgm_version & 0xFF));

	// VGM data offset
	// Header fields must end before this offset.
	unsigned int data_offset;
	if (vgm_version < 0x0150) {
		// VGM older than v1.50: Fixed start offset of 0x40.
		data_offset = 0x40;
	} else {
		data_offset = le32_to_cpu(d->vgmHeader.data_offset) + offsetof(VGM_Header, data_offset);
		// Sanity check: Must be less than 4k.
		assert(data_offset <= 4096);
		if (data_offset > 4096) {
			data_offset = 4096;
		}
	}

	// NOTE: Not byteswapping when checking for 0 because
	// 0 in big-endian is the same as 0 in little-endian.

	// GD3 tags
	if (d->vgmHeader.gd3_offset != 0) {
		// TODO: Make sure the GD3 offset is stored after the header.
		const unsigned int addr = le32_to_cpu(d->vgmHeader.gd3_offset) + offsetof(VGM_Header, gd3_offset);
		VGMPrivate::gd3_tags_t *const gd3_tags = d->loadGD3(addr);
		if (gd3_tags) {
			// TODO: Option to show Japanese instead of English.

			// Array of GD3 tag indexes and translatable strings.
#ifdef ENABLE_NLS
			struct gd3_tag_field_tbl_t {
				const char *ctx;	// Translation context
				const char *desc;	// Description
				GD3_TAG_ID idx;	// GD3 tag index
			};
#define GD3_TAG_FIELD_TBL_ENTRY(ctx, desc, idx) \
			{(ctx), (desc), (idx)}
#else /* !ENABLE_NLS */
			struct gd3_tag_field_tbl_t {
				const char *desc;	// Description
				GD3_TAG_ID idx;		// GD3 tag index
			};
#define GD3_TAG_FIELD_TBL_ENTRY(ctx, desc, idx) \
			{(desc), (idx)}
#endif /* ENABLE_NLS */

			// TODO: Multiple composer handling.
			static const gd3_tag_field_tbl_t gd3_tag_field_tbl[] = {
				GD3_TAG_FIELD_TBL_ENTRY("RomData|Audio",	NOP_C_("RomData|Audio", "Track Name"),	GD3_TAG_TRACK_NAME_EN),
				GD3_TAG_FIELD_TBL_ENTRY("VGM",			NOP_C_("VGM", "Game Name"),		GD3_TAG_GAME_NAME_EN),
				GD3_TAG_FIELD_TBL_ENTRY("VGM",			NOP_C_("VGM", "System Name"),		GD3_TAG_SYSTEM_NAME_EN),
				GD3_TAG_FIELD_TBL_ENTRY("RomData|Audio",	NOP_C_("RomData|Audio", "Composer"),	GD3_TAG_TRACK_AUTHOR_EN),
				GD3_TAG_FIELD_TBL_ENTRY("RomData",		NOP_C_("RomData", "Release Date"),	GD3_TAG_DATE_GAME_RELEASE),
				GD3_TAG_FIELD_TBL_ENTRY("VGM",			NOP_C_("VGM", "VGM Ripper"),		GD3_TAG_VGM_RIPPER),
				GD3_TAG_FIELD_TBL_ENTRY("RomData|Audio",	NOP_C_("RomData|Audio", "Notes"),	GD3_TAG_NOTES),
			};

			for (const auto &p : gd3_tag_field_tbl) {
				const string &str = (*gd3_tags)[p.idx];
				if (!str.empty()) {
					d->fields.addField_string(
						dpgettext_expr(RP_I18N_DOMAIN, p.ctx, p.desc), str);
				}
			}

			delete gd3_tags;
		}
	}

	// Duration [1.00]
	d->fields.addField_string(C_("VGM", "Duration"),
		formatSampleAsTime(le32_to_cpu(vgmHeader->sample_count), VGM_SAMPLE_RATE));

	// Loop point [1.00]
	if (vgmHeader->loop_offset != 0) {
		d->fields.addField_string(C_("VGM", "Loop Offset"),
			formatSampleAsTime(le32_to_cpu(vgmHeader->loop_offset), VGM_SAMPLE_RATE));
	}

	// Framerate. [1.01]
	if (vgm_version >= 0x0101) {
		if (vgmHeader->frame_rate != 0) {
			d->fields.addField_string_numeric(C_("VGM", "Frame Rate"),
				le32_to_cpu(vgmHeader->frame_rate));
		}
	}

	// Sound chips.
	d->fields.addTab(C_("VGM", "Sound Chips"));

	// TODO:
	// - VGM 1.51: Loop modifier
	// - VGM 1.60: Volume modifier, loop base

	// Common strings.
	if (!d->s_clockrate) {
		d->s_clockrate = C_("VGM", "%s Clock Rate");
		d->s_dualchip = C_("VGM", "%s Dual-Chip");
		d->s_yes = C_("RomData", "Yes");
		d->s_no = C_("RomData", "No");
	}
	// Common strings not needed by subroutines.
	const char *const s_flags = C_("VGM", "%s Flags");

	// SN76489 [1.00]
	const uint32_t sn76489_clk = le32_to_cpu(vgmHeader->sn76489_clk);
	if ((sn76489_clk & ~PSG_T6W28) != 0) {
		// TODO: Handle the dual-chip bit.

		// Check for T6W28.
		const bool isT6W28 = ((sn76489_clk & PSG_T6W28) == PSG_T6W28);
		const char *const chip_name = (isT6W28 ? "T6W28" : "SN76489");

		d->fields.addField_string(
			rp_sprintf(d->s_clockrate, chip_name).c_str(),
			LibRpText::formatFrequency(sn76489_clk & ~PSG_T6W28));
		if (!isT6W28) {
			d->fields.addField_string(
				rp_sprintf(d->s_dualchip, chip_name).c_str(),
					(sn76489_clk & VGM_CLK_FLAG_DUALCHIP) ? d->s_yes : d->s_no);
		}

		// LFSR data. [1.10; defaults used for older versions]
		uint16_t lfsr_feedback = 0x0009;
		uint8_t lfsr_width = 16;
		if (vgm_version >= 0x0110) {
			if (vgmHeader->sn76489_lfsr != 0) {
				lfsr_feedback = le16_to_cpu(vgmHeader->sn76489_lfsr);
			}
			if (vgmHeader->sn76489_width != 0) {
				lfsr_width = vgmHeader->sn76489_width;
			}
		}

		d->fields.addField_string_numeric(
			rp_sprintf(C_("VGM", "%s LFSR pattern"), chip_name).c_str(),
			lfsr_feedback, RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);
		d->fields.addField_string_numeric(
			rp_sprintf(C_("VGM", "%s LFSR width"), chip_name).c_str(),
			lfsr_width);

		// Flags. [1.51]
		uint32_t psg_flags;
		if (vgm_version >= 0x0151) {
			// NOTE: Bits 2 and 3 are active low, so invert them here.
			psg_flags = vgmHeader->sn76489_flags ^ 0x0C;
		} else {
			// No PSG flags.
			psg_flags = 0;
		}
		static const char *const psg_flags_bitfield_names[] = {
			NOP_C_("VGM|PSGFlags", "Freq 0 is 0x400"),
			NOP_C_("VGM|PSGFlags", "Output Negate"),
			NOP_C_("VGM|PSGFlags", "Stereo"),
			NOP_C_("VGM|PSGFlags", "/8 Clock Divider"),
		};
		vector<string> *const v_psg_flags_bitfield_names = RomFields::strArrayToVector_i18n(
			"VGM|PSGFlags", psg_flags_bitfield_names, ARRAY_SIZE(psg_flags_bitfield_names));
		d->fields.addField_bitfield(rp_sprintf(s_flags, chip_name).c_str(),
			v_psg_flags_bitfield_names, 2, psg_flags);
	}

	// Macro for sound chips that don't have any special bitflags or parameters.
	// Set dual to true if the sound chip supports dual-chip mode.
#define SOUND_CHIP(field, display, dual) \
	do { \
		if (offsetof(VGM_Header, field##_clk) < data_offset) { \
			d->addCommonSoundChip(le32_to_cpu(vgmHeader->field##_clk), (display), (dual)); \
		} \
	} while (0)

	// YM2413 [1.00]
	SOUND_CHIP(ym2413, "YM2413", true);

	if (vgm_version >= 0x0110) {
		// YM2612 [1.10]
		SOUND_CHIP(ym2612, "YM2612", true);

		// YM2151 [1.10]
		SOUND_CHIP(ym2151, "YM2151", true);
	}

	// TODO: Optimize data offset checks.
	// If e.g. Sega PCM is out of range, the rest of the chips will also
	// be out of range, so we should skip them.
	if (vgm_version >= 0x0151) {
		// Sega PCM [1.51]
		if (offsetof(VGM_Header, sega_pcm_if_reg) < data_offset) {
			unsigned int clk = le32_to_cpu(vgmHeader->sega_pcm_clk);
			clk &= ~VGM_CLK_FLAG_ALTMODE;
			if (clk != 0) {
				d->fields.addField_string(
					rp_sprintf(d->s_clockrate, "Sega PCM").c_str(),
					LibRpText::formatFrequency(clk));
				d->fields.addField_string_numeric(
					rp_sprintf(C_("VGM", "%s IF reg"), "Sega PCM").c_str(),
					le32_to_cpu(vgmHeader->sega_pcm_if_reg),
					RomFields::Base::Hex, 8, RomFields::STRF_MONOSPACE);
			}
		}

		// RF5C68 [1.51]
		SOUND_CHIP(rf5c68, "RF5C68", false);

		// AY8910 flags.
		// Used for YM2203, YM2608, and AY8910.
		static const char *const ay8910_flags_bitfield_names[] = {
			NOP_C_("VGM|AY8910Flags", "Legacy Output"),
			NOP_C_("VGM|AY8910Flags", "Single Output"),
			NOP_C_("VGM|AY8910Flags", "Discrete Output"),
			NOP_C_("VGM|AY8910Flags", "Raw Output"),
		};

		// YM2203 [1.51]
		if (offsetof(VGM_Header, ym2203_ay8910_flags) < data_offset) {
			const unsigned int clk_full = le32_to_cpu(vgmHeader->ym2203_clk);
			const unsigned int clk = clk_full & ~(VGM_CLK_FLAG_ALTMODE | VGM_CLK_FLAG_DUALCHIP);
			if (clk != 0) {
				d->fields.addField_string(
					rp_sprintf(d->s_clockrate, "YM2203").c_str(),
					LibRpText::formatFrequency(clk));
				d->fields.addField_string(
					rp_sprintf(d->s_dualchip, "YM2203").c_str(),
						(clk_full & VGM_CLK_FLAG_DUALCHIP) ? d->s_yes : d->s_no);

				// TODO: Is AY8910 type needed?
				vector<string> *const v_ay8910_flags_bitfield_names = RomFields::strArrayToVector_i18n(
					"VGM|AY8910Flags", ay8910_flags_bitfield_names, ARRAY_SIZE(ay8910_flags_bitfield_names));
				d->fields.addField_bitfield(rp_sprintf(s_flags, "YM2203 (AY8910)").c_str(),
					v_ay8910_flags_bitfield_names, 2, vgmHeader->ym2203_ay8910_flags);
			}
		}

		// YM2608 [1.51]
		if (offsetof(VGM_Header, ym2608_ay8910_flags) < data_offset) {
			const unsigned int clk_full = le32_to_cpu(vgmHeader->ym2608_clk);
			const unsigned int clk = clk_full & ~(VGM_CLK_FLAG_ALTMODE | VGM_CLK_FLAG_DUALCHIP);
			if (clk != 0) {
				d->fields.addField_string(
					rp_sprintf(d->s_clockrate, "YM2608").c_str(),
					LibRpText::formatFrequency(clk));
				d->fields.addField_string(
					rp_sprintf(d->s_dualchip, "YM2608").c_str(),
						(clk_full & VGM_CLK_FLAG_DUALCHIP) ? d->s_yes : d->s_no);

				// TODO: Is AY8910 type needed?
				vector<string> *const v_ay8910_flags_bitfield_names = RomFields::strArrayToVector_i18n(
					"VGM|AY8910Flags", ay8910_flags_bitfield_names, ARRAY_SIZE(ay8910_flags_bitfield_names));
				d->fields.addField_bitfield(rp_sprintf(s_flags, "YM2608 (AY8910)").c_str(),
					v_ay8910_flags_bitfield_names, 2, vgmHeader->ym2608_ay8910_flags);
			}
		}

		// YM2610/YM2610B [1.51]
		if (offsetof(VGM_Header, ym2610_clk) < data_offset) {
			const unsigned int clk_full = le32_to_cpu(vgmHeader->ym2610_clk);
			const unsigned int clk = clk_full & ~(VGM_CLK_FLAG_ALTMODE | VGM_CLK_FLAG_DUALCHIP);
			if (clk != 0) {
				const char *const chip_name =
					(clk_full & VGM_CLK_FLAG_ALTMODE) ? "YM2610B" : "YM2610";

				d->fields.addField_string(
					rp_sprintf(d->s_clockrate, chip_name).c_str(),
					LibRpText::formatFrequency(clk));
				d->fields.addField_string(
					rp_sprintf(d->s_dualchip, chip_name).c_str(),
						(clk_full & VGM_CLK_FLAG_DUALCHIP) ? d->s_yes : d->s_no);
			}
		}

		// YM3812 [1.51]
		SOUND_CHIP(ym3812, "YM3812", true);

		// YM3526 [1.51]
		SOUND_CHIP(ym3526, "YM3526", true);

		// Y8950 [1.51]
		SOUND_CHIP(y8950, "Y8950", true);

		// YMF262 [1.51]
		SOUND_CHIP(ymf262, "YMF262", true);

		// YMF278B [1.51]
		SOUND_CHIP(ymf278b, "YMF278B", true);

		// YMF271 [1.51]
		SOUND_CHIP(ymf271, "YMF271", true);

		// YMZ280B [1.51]
		SOUND_CHIP(ymz280b, "YMZ280B", true);

		// RF5C164 [1.51]
		SOUND_CHIP(rf5c164, "RF5C164", false);

		// PWM [1.51]
		SOUND_CHIP(pwm, "PWM", false);

		// AY8910 [1.51]
		if (offsetof(VGM_Header, ay8910_flags) < data_offset) {
			const unsigned int clk_full = le32_to_cpu(vgmHeader->ay8910_clk);
			const unsigned int clk = clk_full & ~(VGM_CLK_FLAG_ALTMODE | VGM_CLK_FLAG_DUALCHIP);
			if (clk != 0) {
				const char *chip_name;
				// Use a lookup table.
				// Valid bits: xxxCxxBA
				if (!(vgmHeader->ay8910_type & ~0x13)) {
					// Convert to xxxxxCBA.
					uint8_t lkup = vgmHeader->ay8910_type;
					lkup = (lkup >> 2) | (lkup & 3);

					static const char chip_name_tbl[8][8] = {
						"AY8910", "AY8912", "AY8913", "AY8930",
						"YM2139", "YM3439", "YMZ284", "YMZ294",
					};
					chip_name = chip_name_tbl[lkup];
				} else {
					// TODO: Print the type ID?
					chip_name = "AYxxxx";
				}

				d->fields.addField_string(
					rp_sprintf(d->s_clockrate, chip_name).c_str(),
					LibRpText::formatFrequency(clk));
				d->fields.addField_string(
					rp_sprintf(d->s_dualchip, chip_name).c_str(),
						(clk_full & VGM_CLK_FLAG_DUALCHIP) ? d->s_yes : d->s_no);

				vector<string> *const v_ay8910_flags_bitfield_names = RomFields::strArrayToVector_i18n(
					"VGM|AY8910Flags", ay8910_flags_bitfield_names, ARRAY_SIZE(ay8910_flags_bitfield_names));
				d->fields.addField_bitfield(rp_sprintf(s_flags, chip_name).c_str(),
					v_ay8910_flags_bitfield_names, 2, vgmHeader->ay8910_flags);
			}
		}
	}

	if (vgm_version >= 0x0161) {
		// Game Boy (LR35902) [1.61]
		SOUND_CHIP(dmg, "DMG", true);

		// NES APU (2A03) [1.61]
		if (offsetof(VGM_Header, nes_apu_clk) < data_offset) {
			const unsigned int clk_full = le32_to_cpu(vgmHeader->nes_apu_clk);
			const unsigned int clk = clk_full & ~(VGM_CLK_FLAG_ALTMODE | VGM_CLK_FLAG_DUALCHIP);
			if (clk != 0) {
				d->fields.addField_string(
					rp_sprintf(d->s_clockrate, "NES APU").c_str(),
						LibRpText::formatFrequency(clk));
				d->fields.addField_string(
					rp_sprintf(d->s_dualchip, "NES APU").c_str(),
						(clk_full & VGM_CLK_FLAG_DUALCHIP) ? d->s_yes : d->s_no);

				// Bit 31 indicates presence of FDS audio hardware.
				const char *const nes_exp = (clk_full & VGM_CLK_FLAG_ALTMODE)
					? C_("VGM|NESExpansion", "Famicom Disk System")
					: C_("VGM|NESExpansion", "(none)");
				d->fields.addField_string(
					rp_sprintf(C_("VGM", "%s Expansions"), "NES APU").c_str(), nes_exp);
			}
		}

		// MultiPCM [1.61]
		SOUND_CHIP(multipcm, "MultiPCM", true);

		// uPD7759 [1.61]
		SOUND_CHIP(upd7759, "uPD7759", true);

		// NOTE: Ordering is done by the clock rate field,
		// not the flags field.

		// OKIM6258 [1.61]
		// TODO: Flags
		SOUND_CHIP(okim6258, "OKIM6258", true);

		// OKIM6295 [1.61]
		SOUND_CHIP(okim6295, "OKIM6295", true);

		// K051649 [1.61]
		SOUND_CHIP(k051649, "K051649", true);

		// K054539 [1.61]
		// TODO: Flags
		SOUND_CHIP(k054539, "K054539", true);

		// HuC6280 [1.61]
		SOUND_CHIP(huc6280, "HuC6280", true);

		// C140 [1.61]
		// TODO: Flags
		SOUND_CHIP(c140, "C140", true);

		// K053260 [1.61]
		SOUND_CHIP(k053260, "K053260", true);

		// Pokey [1.61]
		SOUND_CHIP(pokey, "Pokey", true);

		// QSound
		SOUND_CHIP(qsound, "QSound", false);
	}

	if (vgm_version >= 0x0171) {
		// SCSP [1.71]
		SOUND_CHIP(scsp, "SCSP", true);

		// WonderSwan [1.71]
		SOUND_CHIP(ws, "WonderSwan", true);

		// VSU-VUE [1.71]
		SOUND_CHIP(vsu, "VSU-VUE", true);

		// SAA1099 [1.71]
		SOUND_CHIP(saa1099, "SAA1099", true);

		// ES5503 [1.71]
		if (offsetof(VGM_Header, es5503_num_ch) < data_offset) {
			const unsigned int clk_full = le32_to_cpu(vgmHeader->es5503_clk);
			const unsigned int clk = clk_full & ~(VGM_CLK_FLAG_ALTMODE | VGM_CLK_FLAG_DUALCHIP);
			if (clk != 0) {
				d->fields.addField_string(
					rp_sprintf(d->s_clockrate, "ES5503").c_str(),
						LibRpText::formatFrequency(clk));
				d->fields.addField_string(
					rp_sprintf(d->s_dualchip, "ES5503").c_str(),
						(clk_full & VGM_CLK_FLAG_DUALCHIP) ? d->s_yes : d->s_no);

				d->fields.addField_string_numeric(
					rp_sprintf(C_("VGM", "%s # of Channels"), "ES5503").c_str(),
						vgmHeader->es5503_num_ch);
			}
		}

		// ES5505/ES5506 [1.71]
		if (offsetof(VGM_Header, es5505_num_ch) < data_offset) {
			const unsigned int clk_full = le32_to_cpu(vgmHeader->es5505_clk);
			const unsigned int clk = clk_full & ~(VGM_CLK_FLAG_ALTMODE | VGM_CLK_FLAG_DUALCHIP);
			if (clk != 0) {
				const char *const chip_name = (clk_full & VGM_CLK_FLAG_ALTMODE)
					? "ES5506"
					: "ES5505";

				d->fields.addField_string(
					rp_sprintf(d->s_clockrate, chip_name).c_str(),
						LibRpText::formatFrequency(clk));
				d->fields.addField_string(
					rp_sprintf(d->s_dualchip, chip_name).c_str(),
						(clk_full & VGM_CLK_FLAG_DUALCHIP) ? d->s_yes : d->s_no);

				d->fields.addField_string_numeric(
					rp_sprintf(C_("VGM", "%s # of Channels"), chip_name).c_str(),
						vgmHeader->es5505_num_ch);
			}
		}

		// X1-010 [1.71]
		SOUND_CHIP(x1_010, "X1-010", true);

		// C352 [1.71]
		if (offsetof(VGM_Header, es5505_num_ch) < data_offset) {
			const unsigned int clk_full = le32_to_cpu(vgmHeader->c352_clk);
			const unsigned int clk = clk_full & ~(VGM_CLK_FLAG_ALTMODE | VGM_CLK_FLAG_DUALCHIP);
			if (clk != 0) {
				d->fields.addField_string(
					rp_sprintf(d->s_clockrate, "C352").c_str(),
						LibRpText::formatFrequency(clk));
				d->fields.addField_string(
					rp_sprintf(d->s_dualchip, "C352").c_str(),
						(clk_full & VGM_CLK_FLAG_DUALCHIP) ? d->s_yes : d->s_no);

				d->fields.addField_string_numeric(
					rp_sprintf(C_("VGM", "%s Clock Divider"), "C352").c_str(),
						vgmHeader->c352_clk_div * 4);
			}
		}

		// GA20 [1.71]
		SOUND_CHIP(ga20, "GA20", true);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int VGM::loadMetaData(void)
{
	RP_D(VGM);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(6);	// Maximum of 6 metadata properties.

	// VGM header.
	const VGM_Header *const vgmHeader = &d->vgmHeader;

	// Length, in milliseconds. (non-looping)
	d->metaData->addMetaData_integer(Property::Duration,
		convSampleToMs(le32_to_cpu(vgmHeader->sample_count), VGM_SAMPLE_RATE));

	// Attempt to load the GD3 tags.
	if (d->vgmHeader.gd3_offset != 0) {
		// TODO: Make sure the GD3 offset is stored after the header.
		const unsigned int addr = le32_to_cpu(d->vgmHeader.gd3_offset) + offsetof(VGM_Header, gd3_offset);
		VGMPrivate::gd3_tags_t *const gd3_tags = d->loadGD3(addr);
		if (gd3_tags) {
			// TODO: Option to show Japanese instead of English.

			// Array of GD3 tag indexes and properties.
			struct gd3_tag_prop_tbl_t {
				Property prop;	// Metadata property index
				uint8_t idx;	// GD3 tag index (GD3_TAG_ID)
			};

			static const gd3_tag_prop_tbl_t gd3_tag_prop_tbl[] = {
				{Property::Title,	GD3_TAG_TRACK_NAME_EN},
				{Property::Album,	GD3_TAG_GAME_NAME_EN},		// NOTE: Not exactly "album"...
				//{Property::SystemName,	GD3_TAG_SYSTEM_NAME_EN),	// FIXME: No property for this...
				{Property::Composer,	GD3_TAG_TRACK_AUTHOR_EN},	// TODO: Multiple composer handling.
				{Property::ReleaseYear,	GD3_TAG_DATE_GAME_RELEASE},
				//{Property::VGMRipper,	GD3_TAG_VGM_RIPPER},		// FIXME: No property for this...

				// NOTE: Property::Comment is assumed to be user-added
				// on KDE Dolphin 18.08.1. Use Property::Description.
				{Property::Description,	GD3_TAG_NOTES},
			};

			for (const auto &p : gd3_tag_prop_tbl) {
				const string &str = (*gd3_tags)[p.idx];
				if (str.empty())
					continue;

				if (p.prop == Property::ReleaseYear) {
					// Special handling for ReleaseYear.

					// Parse the release date.
					// NOTE: Only year is supported.
					int year;
					char chr;
					int s = sscanf((*gd3_tags)[GD3_TAG_DATE_GAME_RELEASE].c_str(), "%04d%c", &year, &chr);
					if (s == 1 || (s == 2 && (chr == '-' || chr == '/'))) {
						// Year seems to be valid.
						// Make sure the number is acceptable:
						// - No negatives.
						// - Four-digit only. (lol Y10K)
						if (year >= 0 && year < 10000) {
							d->metaData->addMetaData_uint(Property::ReleaseYear, (unsigned int)year);
						}
					}
				} else {
					// Standard string property.
					d->metaData->addMetaData_string(p.prop, str);
				}
			}

			delete gd3_tags;
		}
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

}
