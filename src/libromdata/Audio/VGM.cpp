/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * VGM.hpp: VGM audio reader.                                              *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
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

#include "VGM.hpp"
#include "librpbase/RomData_p.hpp"

#include "vgm_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <string>
#include <sstream>
#include <vector>
using std::ostringstream;
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(VGM)

class VGMPrivate : public RomDataPrivate
{
	public:
		VGMPrivate(VGM *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(VGMPrivate)

	public:
		// VGM header.
		// NOTE: **NOT** byteswapped in memory.
		VGM_Header vgmHeader;

		/**
		 * Format an IC clock rate in Hz, kHz, MHz, or GHz.
		 * @param clock_rate Clock rate.
		 * @return IC clock rate, in Hz, kHz, MHz, or GHz. (Three decimal places.)
		 */
		string formatClockRate(unsigned int clock_rate);
};

/** VGMPrivate **/

VGMPrivate::VGMPrivate(VGM *q, IRpFile *file)
	: super(q, file)
{
	// Clear the VGM header struct.
	memset(&vgmHeader, 0, sizeof(vgmHeader));
}

/**
 * Format an IC clock rate in Hz, kHz, MHz, or GHz.
 * @param clock_rate Clock rate.
 * @return IC clock rate, in Hz, kHz, MHz, or GHz. (Three decimal places.)
 */
string VGMPrivate::formatClockRate(unsigned int clock_rate)
{
	// TODO: Rounding?

	if (clock_rate < 1000) {
		// Hz
		return rp_sprintf(C_("VGM", "%u Hz"), clock_rate);
	} else if (clock_rate < 1000000) {
		// kHz
		const unsigned int whole = clock_rate / 1000;
		const unsigned int frac = clock_rate % 1000;
		return rp_sprintf_p(C_("VGM", "%1$u.%2$u kHz"), whole, frac);
	} else if (clock_rate < 1000000000) {
		// MHz
		const unsigned int whole = clock_rate / 1000000;
		const unsigned int frac = (clock_rate / 1000) % 1000;
		return rp_sprintf_p(C_("VGM", "%1$u.%2$u MHz"), whole, frac);
	} else {
		// GHz
		const unsigned int whole = clock_rate / 1000000000;
		const unsigned int frac = (clock_rate / 1000000) % 1000;
		return rp_sprintf_p(C_("VGM", "%1$u.%2$u GHz"), whole, frac);
	}
}

/** VGM **/

/**
 * Read an VGM audio file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
VGM::VGM(IRpFile *file)
	: super(new VGMPrivate(this, file))
{
	RP_D(VGM);
	d->className = "VGM";
	d->fileType = FTYPE_AUDIO_FILE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the VGM header.
	d->file->rewind();
	size_t size = d->file->read(&d->vgmHeader, sizeof(d->vgmHeader));
	if (size != sizeof(d->vgmHeader)) {
		delete d->file;
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->vgmHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->vgmHeader);
	info.ext = nullptr;	// Not needed for VGM.
	info.szFile = 0;	// Not needed for VGM.
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		delete d->file;
		d->file = nullptr;
		return;
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
	if (vgmHeader->magic == cpu_to_le32(VGM_MAGIC)) {
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

	static const char *const sysNames[4] = {
		"Video Game Music",
		"VGM",
		"VGM",
		nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *VGM::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".vgm",
		".vgz",	// TODO: Automatic gzip detection.
		//".vgm.gz",	// NOTE: Windows doesn't support this.

		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *VGM::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		"audio/x-vgm",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int VGM::loadFieldData(void)
{
	RP_D(VGM);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// VGM header.
	const VGM_Header *const vgmHeader = &d->vgmHeader;
	d->fields->reserve(1);	// Maximum of 1 fields.

	// Version number. (BCD)
	unsigned int version = le32_to_cpu(vgmHeader->version);
	d->fields->addField_string(C_("VGM", "Version"),
		rp_sprintf_p(C_("VGM", "%1$x.%2$02x"), version >> 8, version & 0xFF));

	// NOTE: Not byteswapping when checking for 0 because
	// 0 in big-endian is the same as 0 in little-endian.

	/** VGM 1.00 **/

	// TODO: GD3 tags.

	// Duration
	d->fields->addField_string(C_("VGM", "Duration"),
		formatSampleAsTime(le32_to_cpu(vgmHeader->sample_count), VGM_SAMPLE_RATE));

	// Loop point
	if (vgmHeader->loop_offset != 0) {
		d->fields->addField_string(C_("VGM", "Loop Offset"),
			formatSampleAsTime(le32_to_cpu(vgmHeader->loop_offset), VGM_SAMPLE_RATE));
	}

	// SN76489
	if (vgmHeader->sn76489_clk != 0) {
		const uint32_t sn76489_clk = le32_to_cpu(vgmHeader->sn76489_clk);
		// TODO: Dual-chip bit.

		// Check for T6W28.
		const char *chip_name;
		if ((sn76489_clk & 0xC0000000) == 0xC0000000) {
			chip_name = "T6W28";
		} else {
			chip_name = "SN76489";
		}

		d->fields->addField_string(
			rp_sprintf(C_("VGM", "%s clock rate"), chip_name).c_str(),
			d->formatClockRate(sn76489_clk & ~0xC0000000));
	}

	// YM2413
	if (vgmHeader->ym2413_clk != 0) {
		d->fields->addField_string(
			rp_sprintf(C_("VGM", "%s clock rate"), "YM2413").c_str(),
			d->formatClockRate(le32_to_cpu(vgmHeader->ym2413_clk)));
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

}
